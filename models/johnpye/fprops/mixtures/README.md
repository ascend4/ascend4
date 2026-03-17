## UNIFAC Mixture Data Scaffold

This directory is the starting point for the native C-data path for
UNIFAC and later activity-coefficient mixture models.

Current contents:

- `unifac_data.h`
  Immutable/generated source-data declarations.
- `unifac_rundata.h`
  Prepared/runtime declarations.
- `convunifac.py`
  Generator scaffold that parses `../../components.a4l` and can report
  the current database shape or emit generated C source files.
  It can also emit auto-generated name-candidate YAML for the shared
  naming registry.

Current intent:

- keep `components.a4l` as the verification/reference source
- generate native C source data from that reference
- later prepare compact runtime UNIFAC objects from the generated data
- eventually retire the ASCEND-instance dependency from the FPROPS flash
  and activity-coefficient code paths

Typical first checks:

```bash
python3 models/johnpye/fprops/mixtures/convunifac.py --summary
```

```bash
python3 models/johnpye/fprops/mixtures/convunifac.py \
  --emit-dir /tmp/unifac_codegen
```
