# Combat2DC stale-preflight audit

Combat2DC makes no intentional gameplay changes from Combat2CZ/DA/DB.

This pass audits the integrator itself before asking for another Termux run.

Changes:
- removed a root-relative Python test invocation and anchored it to `$ROOT`;
- converted the packaged legacy Combat2CT late-staging regression away from a revision-specific diagnostic string;
- relaxed the Combat2AJ staging regression from exact diagnostic prose to the actual staging guard semantics;
- package identity/log/branch/output names advanced to Combat2DC.

Validation performed before packaging:
- all Python files syntax-compiled;
- every test path referenced by the build script exists;
- shell syntax check passed;
- revision-sensitive assertion scan performed;
- manifest regenerated and verified;
- ZIP integrity verified.
