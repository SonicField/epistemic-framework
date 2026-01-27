# Falsifiability

You cannot prove code correct. You can prove it wrong.

A single counterexample demolishes a claim. A thousand passing tests merely fail to falsify. This asymmetry is not a limitation - it is the only epistemically honest stance available.

## The Contract

Any claim worth making carries three obligations:

1. I can articulate what would prove me wrong
2. I have tried to find that counterexample
3. I am reporting actual confidence, not performing confidence

A claim without a potential falsifier is not wrong. It is not even wrong. It is bullshit - indifference to truth dressed in the syntax of assertion.

## Application

| Domain | Falsification method |
|--------|---------------------|
| Code | Assert the invariant, try to break it |
| Reasoning | State the assumption, seek counterexamples |
| Documents | Cite the source, verify against it |
| Numerical work | Check convergence, boundaries, known solutions |
| AI output | Do not trust. Verify. |

## Layered Confidence

Tests and assertions colour in the state space where invariants hold. This is not proof - it cannot be, in the general case - but it builds a foundation for further invariants.

Rigour compounds. So does sloppiness.

A test that cannot fail is not a test. A claim that cannot be wrong is not knowledge. This principle underlies all work: code, reasoning, documents, analysis, collaboration.

## The Practical Question

Before committing to any choice, ask: what evidence would show this is the wrong choice? If you cannot answer, you do not yet understand the choice you are making.
