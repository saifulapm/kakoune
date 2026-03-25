// Test file to verify tree-sitter query predicates
// Open with: ./tskak test_predicates.rs

// ── #match? @constant "^[A-Z][A-Z\d_]*$" ──
// ALL_CAPS identifiers should get @constant face (not @type or @variable)
const MAX_SIZE: usize = 1024;
const MIN_VALUE: i32 = -100;
const API_VERSION: &str = "v2";

// ── #match? @type "^[A-Z]" ──
// PascalCase identifiers used as types should get @type face
fn process(input: String) -> Vec<u8> {
    let items: Vec<String> = Vec::new();
    let map: HashMap<String, i32> = HashMap::new();
    items
}

// ── #match? @constructor "^[A-Z]" ──
// PascalCase in function call position should get @constructor face
fn constructors() {
    let v = Vec::from([1, 2, 3]);
    let s = String::from("hello");
    let p = Point { x: 1, y: 2 };
}

// ── #any-of? @type.enum.variant.builtin "Some" "None" "Ok" "Err" ──
// Built-in enum variants should get @type.enum.variant.builtin face
fn builtins() -> Option<Result<i32, String>> {
    let x: Option<i32> = Some(42);
    let y: Option<i32> = None;
    let ok: Result<i32, &str> = Ok(1);
    let err: Result<i32, &str> = Err("fail");
    Some(Ok(0))
}

// ── #any-of? @function.builtin ──
// Built-in functions should get @function.builtin face
fn builtin_functions() {
    println!("hello");
    eprintln!("error");
    let v = vec![1, 2, 3];
    format!("x = {}", 42);
    assert!(true);
    assert_eq!(1, 1);
    todo!();
    unimplemented!();
    unreachable!();
}

// ── #any-of? @type.builtin ──
// Built-in types should get @type.builtin face
fn builtin_types(
    a: bool,
    b: char,
    c: str,
    d: u8, e: u16, f: u32, g: u64, h: u128,
    i: i8, j: i16, k: i32, l: i64, m: i128,
    n: f32, o: f64,
    p: usize, q: isize,
) {}

// ── #match? @constant "^[A-Z_]+$" in type_arguments ──
// ALL_CAPS type parameters should get @constant face
struct Container<T, ALLOC> {
    data: Vec<T>,
}

// ── #eq? @comment.unused "_" ──
// Underscore pattern should get @comment.unused face
fn unused_patterns() {
    let _ = 42;
    let (x, _) = (1, 2);
    match Some(1) {
        Some(v) => println!("{}", v),
        _ => {}
    }
}

// ── #eq? @keyword "use" in generic_type ──
// "use" as generic type should get @keyword face
// (rare in practice, but tests #eq? with literal)

// ── #eq? @special "derive" ──
// derive attribute should get @special face
#[derive(Debug, Clone, PartialEq)]
struct Point {
    x: i32,
    y: i32,
}

#[derive(Default)]
enum Color {
    Red,
    Green,
    #[default]
    Blue,
}

// ── Mix of predicate-filtered highlights ──
fn main() {
    const THRESHOLD: f64 = 0.5;        // @constant via #match?
    let result = Ok(42);                // Ok -> @type.enum.variant.builtin via #any-of?
    let name = String::from("world");   // String -> @constructor via #match?
    let count: usize = 0;              // usize -> @type.builtin via #any-of?
    let _ = format!("n={}", count);     // _ -> @comment.unused, format -> @function.builtin

    match result {
        Ok(v) => println!("got {}", v),  // Ok -> builtin variant, println -> builtin fn
        Err(e) => eprintln!("{}", e),    // Err -> builtin variant, eprintln -> builtin fn
    }
}
