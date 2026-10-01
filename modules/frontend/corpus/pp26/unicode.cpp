// langdiff: c++2d
// P3658R1: the mathematical notation profile's characters in identifiers (the paper's table), as Clang 23.1
// lexes them in every mode; one identifier each (tools/checks/langdiff.py).
int ∇f = 1, ∂Ω = 2, C∞ = 3, x² = 4, x₂ = 5, 𝛁g = 6, 𝜕Ω = 7;
int Hawaiʻi = 8, ǃnu = 9, fʹ = 10, grad_𝑓 = 11, xⁿ = 12;
#define NABLA(f) ∇##f
int pasted = NABLA(f);
