# The Argument for C

*Dr Alex Turner and Claude Opus 4.6 — 12 February 2026*

## The Received Wisdom

Rust is the safe language. C is the dangerous one. Every memory safety CVE is an argument for Rust and against C. The borrow checker prevents use-after-free. The type system prevents data races. The compiler enforces what discipline cannot.

This is the standard position. It is not wrong. But it rests on assumptions worth examining — assumptions about who writes the code, how they verify it, and what safety actually means.

## Safety Through Verbs, Not Nouns

Rust's safety is structural. Ownership types, lifetime annotations, `Send` and `Sync` trait markers — these are properties of the code's static description. They are nouns. The compiler checks them before a single instruction executes.

C offers none of this. Every pointer is suspect. Every allocation might leak. Every buffer might overflow. C makes no promises, which means C cannot deceive.

The alternative to structural safety is active safety — verification as action. Assertions at entry, exit, and state transitions. Property-based testing that generates thousands of adversarial inputs. AddressSanitizer, ThreadSanitizer, UndefinedBehaviourSanitizer. Valgrind. Fuzz testing. Integration tests against real systems.

These are verbs. They happened or they did not. Their results are observable, falsifiable, and concrete.

The question is: if you do the verbs properly, what do the nouns still buy you?

## What the Nouns Buy

Rust's compile-time guarantees cover one specific class of bugs — memory safety and data races — across all code paths, including paths your tests never exercise.

This is genuine and non-trivial. Test coverage is always incomplete. ASan only catches bugs on paths it actually runs. A use-after-free on an error path you never tested is invisible to dynamic analysis and visible to the borrow checker.

That is Rust's irreducible advantage: static coverage of memory safety across the entire programme.

## What the Nouns Cost

Type systems constrain solutions toward what the checker can verify, not what the problem demands. They cannot express "this list is sorted", "this connection is authenticated", or "this balance is non-negative." These properties — the ones that actually define correctness — require runtime assertions regardless of language.

Worse, type-system safety creates a false confidence that leaks beyond its domain. "It compiles, therefore it is correct" is not something anyone says explicitly. It is something that happens to verification discipline when the compiler handles half the work. You stop looking as hard. The confidence that memory is safe extends, through habit, to confidence that logic is correct. But the compiler said nothing about logic.

There is also a cost in transparency. Rust's abstractions hide the machine. Ownership, borrowing, lifetimes, trait dispatch — these sit between you and what the processor actually does. For most software this is fine. For performance-critical systems programming, it is a problem.

The terminal weathering project provides a concrete example. A Rust/PyO3 extension was 6% slower than the pure Python it replaced. Not because Rust generated slow code, but because Rust's safety abstractions sat on top of CPython's call protocol dispatch chain, hiding the actual cost structure. The overhead was in the boundary crossings — GIL checks, type checks, borrow checks, refcount clones — roughly 50 nanoseconds per crossing, multiplied across ten million crossings per benchmark run. Five hundred milliseconds of tax, invisible behind the abstraction.

C, by providing no abstraction, exposed the dispatch chain directly. The solution — replacing type slots in CPython's C API — was only visible because C forced you to look at the layer where the problem lived. The replacement was 2.06 times faster than Rust.

## The Discipline Argument

The standard rebuttal: discipline does not scale. Humans get tired, distracted, pressured. They skip the sanitiser run because the deadline is tomorrow. They forget the assertion because the function is "obviously correct." Rust encodes safety in the compiler because humans cannot be trusted to maintain discipline consistently across a codebase, across a team, across years.

This is empirically sound for human teams. It is also the wrong frame for the present moment.

## The AI Shift

AI agents do not get tired. They do not get annoyed at the build system. They do not skip the sanitiser run because it is Friday afternoon. They execute the verification cycle identically on the thousandth iteration as the first.

More precisely:

- They run ASan, TSan, UBSan, and Valgrind on every build, every time, without complaint.
- They generate adversarial test inputs exhaustively, not until boredom sets in.
- They write assertions at three levels — preconditions, postconditions, invariants — because the protocol says to, and they follow protocols.
- They scale horizontally. Ten agents verifying in parallel costs no more attention than one.
- They do not have bad days.

The discipline argument against C was always economic, not epistemological. "We cannot afford to verify everything manually, so let the compiler do some of it." AI zeroes out the cost of the verbs. When verification is effectively free, paying Rust's complexity tax to avoid verification is paying for something you no longer need.

## The Epistemic Argument

There is a stronger claim than "C is adequate with proper verification." The stronger claim: C is *better* for verified development, precisely because it makes no guarantees.

A language that promises nothing forces you to verify everything. Every pointer, every allocation, every concurrent access, every return value. You cannot rely on the compiler's judgement because it offers none. This produces an epistemic posture of permanent suspicion — which is exactly the posture that catches bugs.

Rust permits selective trust. The compiler handles memory safety, so you direct your attention elsewhere. But attention is not infinitely divisible. When you stop looking for memory bugs, you may also — through habit, through the shape of your workflow — stop looking as hard for the logical bugs that live next door. The trust boundary bleeds.

C has no trust boundary. Everything is suspect. The verification discipline it demands covers memory *and* logic *and* concurrency *and* performance, because it cannot afford to assume any of these are handled.

This is the Peltzman effect applied to programming languages: safety features cause compensating changes in behaviour. Remove the safety feature; behaviour adjusts; total safety may not decrease.

The population-level counterargument — that most C developers lack the discipline, so Rust's floor prevents worse outcomes — does not apply when the developers are AI agents operating under a formal verification protocol. You are not reasoning about a population with variable discipline. You are reasoning about a system with defined, repeatable, invariant behaviour.

## The Falsifiers

Any honest argument states what would prove it wrong.

**This argument fails if**: there exists a class of bugs that AI agents systematically fail to catch with C plus the full verb set (assertions, property-based testing, sanitisers, fuzz testing, integration tests), where Rust's compiler would have caught them. Not occasionally — systematically. A consistent blind spot in AI-driven verification that the borrow checker covers.

**This argument also fails if**: the economic cost of AI verification exceeds the cognitive cost of Rust's type system for the problems in question. If fighting the borrow checker takes less total compute than running exhaustive dynamic analysis, Rust wins on efficiency even if C wins on transparency.

**This argument does not apply when**: you are fielding a human team without AI support. For human teams, Rust's guarantees remain valuable precisely because discipline is expensive and inconsistent. Nothing here argues against Rust for human-scale development.

## The Position

C makes no promises. In a world where verification is cheap and abundant — where AI agents execute the full protocol every time, without fatigue, at scale — the absence of promises is not a weakness. It is honesty. And honesty, in engineering as in epistemology, is the foundation of trust.

Rust's value was always as a substitute for discipline. When discipline is free, the substitution is unnecessary. What remains is a complexity tax on a language that hides the machine, in a domain where seeing the machine is the point.

Write C. Verify everything. Trust nothing the compiler did not check — and trust nothing the compiler claims to have checked, either. Safety comes from verbs, not nouns.
