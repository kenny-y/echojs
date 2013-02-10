esprima = require 'esprima'
syntax = esprima.Syntax
escodegen = require 'escodegen'
debug = require 'debug'

{ Set } = require 'set'
{ NodeVisitor } = require 'nodevisitor'
{ genGlobalFunctionName, genAnonymousFunctionName, deep_copy_object, map, foldl } = require 'echo-util'

decl_names = (arr) ->
        result = []
        for n in arr
                if n.declarations?
                        result = result.concat (decl.id.name for decl in n.declarations)
                else if n.id?
                        result.push n.id.name
        new Set result

param_names = (arr) ->
        new Set map ((id) -> id.name), arr

collect_decls = (body) ->
       statement for statement in body when statement.type is syntax.VariableDeclaration or statement.type is syntax.FunctionDeclaration

# TODO: convert this to a visitor
free_blocklike = (exp,body) ->
        decls = decl_names collect_decls body
        uses = Set.union.apply null, map free, body
        exp.ejs_decls = decls
        exp.ejs_free_vars = uses.subtract decls
        exp.ejs_free_vars
        
exports.free = free = (exp) ->
        if not exp?
                return new Set
        switch exp.type
                when syntax.Program
                        decls = decl_names collect_decls exp.body
                        uses = Set.union.apply null, (map free, exp.body)
                        exp.ejs_decls = decls
                        exp.ejs_free_vars = uses.subtract decls
                when syntax.FunctionDeclaration
                        exp.ejs_free_vars = (free exp.body).subtract (param_names exp.params)
                        exp.ejs_decls = exp.body.ejs_decls.union (param_names exp.params)
                        exp.ejs_free_vars
                when syntax.FunctionExpression
                        exp.ejs_free_vars = (free exp.body).subtract (param_names exp.params)
                        exp.ejs_decls = exp.body.ejs_decls.union (param_names exp.params)
                        exp.ejs_free_vars
                when syntax.LabeledStatement      then exp.ejs_free_vars = free exp.body
                when syntax.BlockStatement        then exp.ejs_free_vars = free_blocklike exp, exp.body
                when syntax.TryStatement          then exp.ejs_free_vars = Set.union.apply null, [(free exp.block)].concat (map free, exp.handlers)
                when syntax.CatchClause
                        param_set = if exp.param?.name? then new Set [exp.param.name] else new Set
                        exp.ejs_free_vars = (free exp.body).subtract param_set
                        exp.ejs_decls = exp.body.ejs_decls.union param_set
                        exp.ejs_free_vars
                when syntax.VariableDeclaration   then exp.ejs_free_vars = Set.union.apply null, (map free, exp.declarations)
                when syntax.VariableDeclarator    then exp.ejs_free_vars = free exp.init
                when syntax.ExpressionStatement   then exp.ejs_free_vars = free exp.expression
                when syntax.Identifier            then exp.ejs_free_vars = new Set [exp.name]
                when syntax.ThrowStatement        then exp.ejs_free_vars = free exp.argument
                when syntax.ForStatement          then exp.ejs_free_vars = Set.union.apply null, (map free, [exp.init, exp.test, exp.update, exp.body])
                when syntax.ForInStatement        then exp.ejs_free_vars = Set.union.apply null, (map free, [exp.left, exp.right, exp.body])
                when syntax.WhileStatement        then exp.ejs_free_vars = Set.union.apply null, (map free, [exp.test, exp.body])
                when syntax.DoWhileStatement      then exp.ejs_free_vars = Set.union.apply null, (map free, [exp.test, exp.body])
                when syntax.SwitchStatement       then exp.ejs_free_vars = Set.union.apply null, [free exp.discriminant].concat (map free, exp.cases)
                when syntax.SwitchCase            then exp.ejs_free_vars = free_blocklike exp, exp.consequent
                when syntax.EmptyStatement        then exp.ejs_free_vars = new Set
                when syntax.BreakStatement        then exp.ejs_free_vars = new Set
                when syntax.ContinueStatement     then exp.ejs_free_vars = new Set
                when syntax.UpdateExpression      then exp.ejs_free_vars = free exp.argument
                when syntax.ReturnStatement       then exp.ejs_free_vars = free exp.argument
                when syntax.UnaryExpression       then exp.ejs_free_vars = free exp.argument
                when syntax.BinaryExpression      then exp.ejs_free_vars = (free exp.left).union free exp.right
                when syntax.LogicalExpression     then exp.ejs_free_vars = (free exp.left).union free exp.right
                when syntax.MemberExpression      then exp.ejs_free_vars = free exp.object # we don't traverse into the property
                when syntax.CallExpression        then exp.ejs_free_vars = Set.union.apply null, [(free exp.callee)].concat (map free, exp.arguments)
                when syntax.NewExpression         then exp.ejs_free_vars = Set.union.apply null, [(free exp.callee)].concat (map free, exp.arguments)
                when syntax.SequenceExpression    then exp.ejs_free_vars = Set.union.apply null, map free, exp.expressions
                when syntax.ConditionalExpression then exp.ejs_free_vars = Set.union.apply null, [(free exp.test), (free exp.cconsequent), free (exp.alternate)]
                when syntax.Literal               then exp.ejs_free_vars = new Set
                when syntax.ThisExpression        then exp.ejs_free_vars = new Set
                when syntax.Property              then exp.ejs_free_vars = free exp.value # we skip the key
                when syntax.ObjectExpression
                        exp.ejs_free_vars = if exp.properties.length is 0 then (new Set) else Set.union.apply null, map free, (p.value for p in exp.properties)
                when syntax.ArrayExpression
                        exp.ejs_free_vars = if exp.elements.length is 0 then (new Set) else Set.union.apply null, map free, exp.elements
                when syntax.IfStatement           then exp.ejs_free_vars = Set.union.apply null, [(free exp.test), (free exp.consequent), free (exp.alternate)]
                when syntax.AssignmentExpression  then exp.ejs_free_vars = (free exp.left).union free exp.right
                
                else throw "Internal error: unhandled node type '#{exp.type}' in free()"
                #else new Set

#
# each element in env is an object of the form:
#   { exp: ...     // the AST node corresponding to this environment.  will be a function or a BlockStatement. (right now just functions)
#     id:  ...     // the number/id of this environment
#     decls: ..    // a set of the variable names that are declared in this environment
#     closed: ...  // a set of the variable names that are used from this environment (i.e. the ones that need to move to the environment)
#     parent: ...  // the parent environment object
#   }
# 
LocateEnv = class LocateEnv extends NodeVisitor
        constructor: ->
                super
                @envs = []
                @env_id = 0
                
        visitFunction: (n) ->
                current_env = @envs[0]
                new_env = { id: @env_id, decls: (if n.ejs_decls? then n.ejs_decls else new Set), closed: new Set, parent: current_env }
                @env_id += 1
                n.ejs_env = new_env
                @envs.unshift new_env
                # don't visit the parameters for the same reason we don't visit the id's of variable declarators below:
                # we don't want a function to look like:  function foo (%env_1.x) { }
                # instead of                              function foo (x) { }
                n.body = @visit n.body
                @envs.shift()
                n

        visitVariableDeclarator: (n) ->
                # don't visit the id, as that will cause us to mark it ejs_substitute = true, and we'll end up with things like:
                #   var %env_1.x = blah
                # instead of what we want:
                #   var x = blah
                n.init = @visit n.init
                n

        visitProperty: (n) ->
                # we don't visit property keys here
                n.value = @visit n.value
                n

        visitIdentifier: (n) ->
                # find the environment in the env-stack that includes this variable's decl.  add it to the env's .closed set.
                current_env = @envs[0]
                env = current_env

                # if the current environment declares that identifier, nothing to do.
                return n if env.decls.member n.name

                closed_over = false
                # look up our environment stack for the decl for it.
                env = env.parent
                while env?
                        if env.decls?.member n.name
                                closed_over = true
                                env = null
                        else
                                env = env.parent

                # if we found it higher on the function stack, we need to walk back up the stack forcing environments along the way, and make
                # sure the frame that declares it knows that something down the stack closes over it.
                if closed_over
                        env = current_env.parent
                        while env?
                                if env.decls?.member n.name
                                        env.closed.add n.name
                                        env = null
                                else
                                        env.nested_requires_env = true
                                        env = env.parent
                
                n.ejs_substitute = true
                n

                ###                
                while env?
                        if env.decls.member n.name
                                if env.closed.member n.name
                                        n.ejs_substitute = true
                                else if env isnt current_env
                                        env.closed.add n.name
                                        n.ejs_substitute = true
#                                env.closed.add n.name
#                                n.ejs_substitute = true
                                return n
                        else
                                env = env.parent
                ###
                n

create_identifier = (x) -> type: syntax.Identifier, name: x
create_string_literal = (x) -> type: syntax.Literal, value: x, raw: "\"#{x}\""

# this should move to echo-desugar.coffee

class HoistFuncDecls extends NodeVisitor
        constructor: ->
                @prepends = []
                
        visitFunction: (n) ->
                @prepends.unshift []
                super
                # we're assuming n.body is a BlockStatement here...
                n.body.body = @prepends.shift().concat n.body.body
                n
        
        visitBlock: (n) ->
                super

                new_body = []
                for child in n.body
                        if child.type is syntax.FunctionDeclaration
                                @prepends[0].push child
                        else
                                new_body.push child
                n.body = new_body
                n

# convert all function declarations to variable assignments
# with named function expressions.
# 
# i.e. from:
#   function foo() { }
# to:
#   var foo = function foo() { }
# 
class FuncDeclsToVars extends NodeVisitor
        visitFunctionDeclaration: (n) ->
                if n.toplevel
                        super
                else
                        func_exp = n
                        func_exp.type = syntax.FunctionExpression
                        func_exp.body = @visit func_exp.body
                        return {
                                type: syntax.VariableDeclaration,
                                declarations: [{
                                        type: syntax.VariableDeclarator
                                        id: create_identifier n.id.name
                                        init: func_exp
                                }],
                                kind: "var"
                        }


# hoists all vars to the start of the enclosing function, replacing
# any initializer with an assignment expression.  We also take the
# opportunity to convert the vars to lets at this point so by the time
# the LLVMIRVisitory gets to the tree there are only consts and lets
#
# i.e. from
#    {
#       ....
#       var x = 5;
#       ....
#    }
# to
#    {
#       let x;
#       ....
#       x = 5;
#       ....
#    }
# 
class HoistVars extends NodeVisitor
        constructor: () ->
                super
                @function_stack = []

        visitFunction: (n) ->
                @function_stack.unshift { func: n, vars: [] }
                n = super
                n.body.body = @function_stack[0].vars.concat n.body.body
                @function_stack.shift()
                n

        visitForIn: (n) ->
                if n.left.type is syntax.VariableDeclaration
                        @function_stack[0].vars.push
                                type: syntax.VariableDeclaration
                                declarations: [{
                                        type: syntax.VariableDeclarator
                                        id: create_identifier n.left.declarations[0].id.name
                                        init: null
                                }],
                                kind: "let"
                        n.left = create_identifier n.left.declarations[0].id.name

                n.right = @visit n.right
                n.body = @visit n.body
                n
                        
        visitVariableDeclaration: (n) ->
                if n.kind is "var"
                        # vars are hoisted to the containing function's toplevel scope
                        for i in [0...n.declarations.length]
                                @function_stack[0].vars.push
                                        type: syntax.VariableDeclaration
                                        declarations: [{
                                                type: syntax.VariableDeclarator
                                                id: create_identifier n.declarations[i].id.name
                                                init: null
                                                }],
                                        kind: "let"

                        # now we need convert the initializers to assignment expressions
                        assignments = []
                        for i in [0...n.declarations.length]
                                if n.declarations[i].init?
                                        assignments.push
                                                type: syntax.ExpressionStatement
                                                expression:
                                                        type: syntax.AssignmentExpression
                                                        left: create_identifier n.declarations[i].id.name
                                                        right: n.declarations[i].init
                                                        operator: "="

                        # now return the new assignments, which will replace the original variable
                        # declaration node.
                        if assignments.length == 1
                                return assignments[0]

                        return {
                                type: syntax.BlockStatement
                                body: assignments
                        }
                n

# this class really doesn't behave like a normal NodeVisitor, as it modifies the tree in-place
# check the free() function near the top of the file for the actual implementation.
class ComputeFree extends NodeVisitor
        visit: (n) ->
                free n
                n
                
# 1) allocates the environment at the start of the n
# 2) adds mappings for all .closed variables
class SubstituteVariables extends NodeVisitor
        constructor: (module) ->
                super
                @function_stack = []
                @mappings = []

        currentMapping: -> if @mappings.length > 0 then @mappings[0] else {}

        visitIdentifier: (n) ->
                if @currentMapping().hasOwnProperty n.name
                        return @currentMapping()[n.name]
                n

        visitFor: (n) ->
                # for loops complicate things.
                # if any of the variables declared in n.init are closed over
                # we promote all of them outside of n.init.

                init = @visit n.init
                n.test = @visit n.test
                n.update = @visit n.update
                n.body = @visit n.body
                if Array.isArray init
                        n.init = null
                        return {
                                type: syntax.BlockStatement
                                body: init.concat [n]
                        }
                else
                        n.init = init
                        n

        visitForIn: (n) ->
                # for-in loops complicate things.

                left = @visit n.left
                n.right = @visit n.right
                n.body = @visit n.body
                if Array.isArray left
                        n.left = create_identifier left[0].declarations[0].id.name
                        return {
                                type: syntax.BlockStatement
                                body: left.concat [n]
                        }
                else
                        n.left = left
                        n


        visitVariableDeclaration: (n) ->
                rv = []
                for decl in n.declarations
                        decl.init = @visit decl.init
                        if @currentMapping().hasOwnProperty decl.id.name
                                rv.push {
                                        type: syntax.ExpressionStatement,
                                        expression:
                                                type: syntax.AssignmentExpression,
                                                operator: "="
                                                left:
                                                        type: syntax.MemberExpression,
                                                        computed: false,
                                                        object: @currentMapping()["%env"]
                                                        property: create_identifier decl.id.name
                                                right: decl.init || { type: syntax.Identifier, name: "undefined" }
                                }
                        else
                                rv.push {
                                        type: syntax.VariableDeclaration
                                        declarations: [decl]
                                        kind: n.kind
                                }
                rv                        

        visitProperty: (n) ->
                # we don't visit property keys here
                n.value = @visit n.value
                n

        visitFunctionBody: (n) ->
                n.scratch_size = 0
                
                # we use this instead of calling super in visitFunction because we don't want to visit parameters
                # during this pass, or they'll be substituted with an %env.

                @function_stack.unshift n
                n.body = @visit n.body
                @function_stack.shift()
                
                n
                
        visitFunction: (n) ->
                if n.ejs_env.closed.empty() and not n.ejs_env.nested_requires_env
                        n.body.body.unshift
                                type: syntax.VariableDeclaration,
                                declarations: [{
                                        type: syntax.VariableDeclarator
                                        id:  create_identifier "%env_#{n.ejs_env.id}"
                                        init:
                                                type: syntax.Literal
                                                value: null
                                }],
                                kind: "let"
                        @visitFunctionBody n
                else
                        # okay, we know we need a fresh environment in this function
                        
                        env_name = "%env_#{n.ejs_env.id}"

                        prepends = []
                        # insert environment creation (at the start of the function body)
                        prepends.push {
                                type: syntax.VariableDeclaration,
                                declarations: [{
                                        type: syntax.VariableDeclarator
                                        id:  create_identifier env_name
                                        init:
                                                type: syntax.ObjectExpression
                                                properties: []
                                }],
                                kind: "let"
                        }

                        if n.ejs_env.parent?
                                prepends.push {
                                        type: syntax.ExpressionStatement
                                        expression:
                                                type: syntax.AssignmentExpression,
                                                operator: "="
                                                left:
                                                        type: syntax.MemberExpression,
                                                        computed: false,
                                                        object: create_identifier env_name
                                                        property: create_identifier "%env_#{n.ejs_env.parent.id}"
                                                right: create_identifier "%env_#{n.ejs_env.parent.id}"
                                }
                                

                        # we need to push assignments of any closed over parameters into the environment at this point
                        for param in n.params
                                if n.ejs_env.closed.member param.name
                                        prepends.push {
                                                type: syntax.ExpressionStatement
                                                expression:
                                                        type: syntax.AssignmentExpression,
                                                        operator: "="
                                                        left:
                                                                type: syntax.MemberExpression,
                                                                computed: false,
                                                                object: create_identifier env_name
                                                                property: create_identifier param.name
                                                        right: create_identifier param.name
                                        }

                        new_mapping = deep_copy_object @currentMapping()

                        flatten_memberexp = (exp) ->
                                if exp.type isnt syntax.MemberExpression
                                        [exp]
                                else
                                        (flatten_memberexp exp.object).concat [exp.property]

                        prepend_environment = (exps) ->
                                obj = create_identifier env_name
                                for prop in exps
                                        obj = {
                                                type: syntax.MemberExpression,
                                                computed: false,
                                                object: obj
                                                property: prop
                                        }
                                obj

                        # if there are existing mappings prepend "%env." (a MemberExpression) to them
                        for mapped of new_mapping
                                val = new_mapping[mapped]
                                new_mapping[mapped] = prepend_environment flatten_memberexp val
                        
                        # add mappings for all variables in .closed from "x" to "%env.x"

                        new_mapping["%env"] = create_identifier env_name
                        n.ejs_env.closed.map (sym) ->
                                new_mapping[sym] =
                                        type: syntax.MemberExpression,
                                        computed: false,
                                        object: create_identifier env_name
                                        property: create_identifier sym

                        @mappings.unshift new_mapping
                        @visitFunctionBody n
                        n.body.body = prepends.concat n.body.body
                        @mappings = @mappings.slice(1)

                # convert function expressions to an explicit closure creation, so:
                # 
                #    function X () { ...body... }
                #
                # replace inline with:
                #
                #    makeClosure(%current_env, "X", function X () { ...body... })

                if not n.toplevel
                        if n.type is syntax.FunctionDeclaration
                                throw "there should be no FunctionDeclarations at this point"
                        else # n.type is syntax.FunctionExpression
                                if n.id?
                                        call_exp =
                                                type: syntax.CallExpression,
                                                callee: create_identifier "%makeClosure"
                                                arguments: [ (if n.ejs_env.parent? then (create_identifier "%env_#{n.ejs_env.parent.id}") else { type: syntax.Literal, value: null}), (create_string_literal n.id.name), n ]
                                else
                                        call_exp =
                                                type: syntax.CallExpression,
                                                callee: create_identifier "%makeAnonClosure"
                                                arguments: [ (if n.ejs_env.parent? then (create_identifier "%env_#{n.ejs_env.parent.id}") else { type: syntax.Literal, value: null}), n ]
                                
                                return call_exp
                n        
                
        visitCallExpression: (n) ->
                super

                # replace calls of the form:
                #   X (arg1, arg2, ...)
                # 
                # with
                #   invokeClosure(X, %this, %argCount, arg1, arg2, ...);

                @function_stack[0].scratch_size = Math.max @function_stack[0].scratch_size, n.arguments.length

                n.arguments.unshift n.callee
                n.callee = create_identifier "%invokeClosure"
                n

        visitNewExpression: (n) ->
                super

                # replace calls of the form:
                #   new X (arg1, arg2, ...)
                # 
                # with
                #   invokeClosure(X, %this, %argCount, arg1, arg2, ...);

                @function_stack[0].scratch_size = Math.max @function_stack[0].scratch_size, n.arguments.length

                n.arguments.unshift n.callee
                n.callee = create_identifier "%invokeClosure"
                n

#
#   This pass walks the tree and moves all function expressions to the toplevel.
#
#   at the point where this pass runs there are a couple of assumptions:
# 
#   1. there are no function declarations anywhere in the program.  They have all
#      been converted to 'var X = %makeClosure(%env_Y, function (%env) { ... })'
# 
#   2. There are no free variables in the function expressions.
#
class LambdaLift extends NodeVisitor
        constructor: (@filename) ->
                super
                @functions = []
                @mappings = []

        currentMapping: -> if @mappings.length > 0 then @mappings[0] else {}

        visitProgram: (program) ->
                super
                program.body = @functions.concat program.body
                program

        maybePrependScratchArea: (n) ->
                if n.scratch_size > 0
                        alloc_scratch =
                                type: syntax.ExpressionStatement
                                expression:
                                        type: syntax.CallExpression
                                        callee: create_identifier "%createArgScratchArea"
                                        arguments: [ { type: syntax.Literal, value: n.scratch_size } ]
                        n.body.body = [alloc_scratch].concat n.body.body
                
        visitFunctionDeclaration: (n) ->
                n.body = @visit n.body
                @maybePrependScratchArea n
                n
        
        visitFunctionExpression: (n) ->
                if n.displayName?
                        global_name = genGlobalFunctionName n.displayName, @filename
                else if n.id?.name?
                        global_name = genGlobalFunctionName n.id.name, @filename
                else
                        global_name = genAnonymousFunctionName(@filename)
                
                if n.id?.name?
                        @currentMapping()[n.id.name] = global_name

                n.type = syntax.FunctionDeclaration
                n.id =
                        type: syntax.Identifier
                        name: global_name

                @functions.push n

                new_mapping = deep_copy_object @currentMapping()

                @mappings.unshift new_mapping
                n.body = @visit n.body
                @mappings = @mappings.slice(1)

                @maybePrependScratchArea n

                n.params.unshift create_identifier "%env_#{n.ejs_env.parent.id}"
                
                return {
                        type: syntax.Identifier,
                        name: global_name
                }

class NameAnonymousFunctions extends NodeVisitor
        visitAssignmentExpression: (n) ->
                n = super n
                lhs = n.left
                rhs = n.right

                # if we have the form
                #   <identifier> = function () { }
                # convert to:
                #   <identifier> = function <identifier> () { }
                #if lhs.type is syntax.Identifier and rhs.type is syntax.FunctionExpression and not rhs.id?.name
                #        rhs.display = <something pretty about the lhs>
                #n
                if rhs.type is syntax.FunctionExpression and not rhs.id?.name
                        rhs.displayName = escodegen.generate lhs
                n

transform_dump_tree = (n, indent=0) ->
        stringified_indent = ('' for i in [0..(indent*2)]).join('| ')
        console.log "#{stringified_indent}.type = syntax.#{n.type}"
        console.log "#{stringified_indent}.js = #{escodegen.generate n, { format: { compact: true } } }"
        console.log "#{stringified_indent}.free = #{n.ejs_free_vars}" if n.ejs_free_vars?
        console.log "#{stringified_indent}.env.decls = #{n.ejs_env.decls}" if n.ejs_env?
        console.log "#{stringified_indent}.env.closed = #{n.ejs_env.closed}" if n.ejs_env?
        props_to_skip = ["type", "id", "loc", "ejs_free_vars", "ejs_decls", "ejs_env"]
        for propname in Object.getOwnPropertyNames n
                if (props_to_skip.indexOf propname) is -1
                        if n[propname]?.type?
                                console.log "#{stringified_indent}.#{propname}:"
                                transform_dump_tree n[propname], indent+1
                        else if Array.isArray n[propname]
                                console.log "#{stringified_indent}.#{propname}: ["
                                (transform_dump_tree element, indent+1) for element in n[propname]
                                console.log "#{stringified_indent}]"
                        else
                                console.log "#{stringified_indent}.#{propname} = #{JSON.stringify n[propname]}"
        undefined

passes = [
        HoistFuncDecls,
        FuncDeclsToVars,
        HoistVars,
        ComputeFree,
        LocateEnv,
        NameAnonymousFunctions,
        SubstituteVariables,
        LambdaLift
        ]

exports.convert = (tree, filename) ->
        debug.log "before:"
        debug.log -> escodegen.generate tree

        passes.forEach (passType) ->
                pass = new passType(filename)
                tree = pass.visit tree
        tree
