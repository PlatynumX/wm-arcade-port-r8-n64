# Combat2DX post-integration path audit

Combat2DW failed after the source-exact DRONE seek patch had already applied because `test_combat2bl_drone_seek_source.py` interpreted `$WORK` as the integrator root and tried to open a package-only patcher under the clone. DX separates patch-source unit checks from integrated-result checks for BL, BO, CZ, and ANI_CODE completion and adds a static contract test covering every test invoked as `test ... "$WORK"`. No runtime behavior is changed.
