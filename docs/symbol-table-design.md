# Symbol Table Design

## What a symbol table does

A symbol table is the compiler's record of declared names. Later, the semantic analyzer can ask whether a name exists, which type it has, where it was declared, and whether it is visible at the current point in the program. For example, after `int count;`, the table records `count` as an integer declaration.

## Entries and scopes

Each `SymbolEntry` stores the variable name, `AstType` (`int`, `float`, or `bool`), scope ID, and declaration line. Entries are kept in a linked list inside their scope.

A scope is the region whose declarations are visible together. The program's outermost declarations belong to root scope 0. A brace block creates a child scope. Nested blocks create scopes below their containing block, and an `if`/`while` body gets a scope because the parser represents each body as an `AST_STMT_BLOCK`.

This project permits **shadowing**: a child scope may declare a name already declared in a parent. A lookup in that child finds the child's declaration first; after leaving it, lookup again finds the parent declaration. Two declarations with the same name in the same scope are duplicates and the second is not inserted. This resolves the shadowing policy left open by the manual.

## Scope tree and operations

`src/symbol_table/symbol_table.c` stores scopes as a tree. Every scope keeps a parent pointer, child list, unique ID, opening location, and its declarations. The table also keeps a `current` pointer.

- `symbol_table_enter_scope` creates a child and makes it current.
- `symbol_table_exit_scope` moves current back to its parent. It refuses to move above root.
- `symbol_table_lookup_local` checks only the current scope.
- `symbol_table_lookup_parent` starts at the current scope's parent.
- `symbol_table_lookup` checks current first, then walks outward until it finds a match or reaches root.
- `symbol_table_declare` checks only current scope before inserting, so shadowing works while same-scope duplicates are rejected.
- `symbol_table_print` walks the retained scope tree, including scopes already exited, which is useful for debugging.
- `symbol_table_destroy` recursively frees entries and scopes.

Exited scopes remain allocated under their parent so the complete table can be printed and destroyed later. They are not searched by normal lookup because lookup walks only from the active scope toward its ancestors.

## Building scopes from the AST

`symbol_table_build_from_ast` walks the actual `AstProgram` and statements. It inserts declarations in the current scope, enters a scope when it visits a block, and exits after the block's statements. For `if` and `while`, it visits their block children, so declarations inside those bodies receive their own scope IDs. It reports duplicate declarations with the declaration line and scope ID. Assignments, expressions, and print statements do not add symbol entries.

The scope tree retains a link to each AST block that created a scope. `symbol_table_enter_existing_block` lets a later AST walk re-enter that same scope, then use ordinary lookup. The AST owns the block nodes; the scope's block reference is borrowed and is not freed by the symbol table.

This build helper gathers declarations for table display and tests. It does not enforce declaration-before-use. When semantic analysis is added, it should walk statements in source order and call `symbol_table_declare` when it reaches each declaration, using enter/exit around blocks. That lets it report a use before a declaration instead of seeing declarations collected from later in the block.

## Example

For:

```text
int x;
{
    float y;
    {
        bool z;
    }
}
```

The root scope contains `x`; its child contains `y`; the child's child contains `z`. While in the innermost block, lookup can find `z`, then `y`, then `x`. After exiting both blocks at the root, `y` and `z` are no longer visible. Their entries remain in the retained tree for printing.

## Build and test

Build the table demonstration program:

```sh
make symbol-table
build/tactician-symbol-table.exe tests/symbol_table/nested_scopes.tac
```

Run all lexer, parser, AST, and symbol-table tests:

```sh
make test
```

The C API checks in `tests/symbol_table_unit.c` exercise insertion of all three types, lookup, missing names, duplicate handling, shadowing, parent resolution, nested enter/exit, sibling isolation, printing, and AST-block scope re-entry. Source-level examples and expected table output are in `tests/symbol_table/`.
