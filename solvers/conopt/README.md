# CONOPT licensing

ASCEND can register a CONOPT 4 license when it creates a CONOPT handle. This
applies both to the direct CONOPT solver and to CONOPT solves initiated by
CMSlv. No license data is stored in an ASCEND model or in the source tree.

The license is one value with this format:

```text
LicString,LicInt1,LicInt2,LicInt3
```

ASCEND splits the value at the last three commas, so `LicString` may contain
commas and does not need an escaping convention.

## Environment variable

Set `ASCEND_CONOPT_LICENSE` in the environment that launches ASCEND:

```sh
export ASCEND_CONOPT_LICENSE='LicString,LicInt1,LicInt2,LicInt3'
./a4 cutest solver_conopt
```

The `a4` helper passes its environment to the programs it launches. Avoid
putting the real value on a command line, in a model, or in a checked-in shell
script.

## User secrets file

Alternatively, create the following file:

- Linux and other Unix-like systems:
  `$XDG_CONFIG_HOME/ascend/secrets.ini`, or
  `~/.config/ascend/secrets.ini` when `XDG_CONFIG_HOME` is unset.
- Windows: `%APPDATA%/ascend/secrets.ini`.

Its contents are:

```ini
[conopt]
license = LicString,LicInt1,LicInt2,LicInt3
```

On Unix-like systems the file must be a regular file owned by the current user
with no group or other permissions. Mode `0600` is recommended. ASCEND refuses
an insecure file rather than passing its contents to CONOPT.

`ASCEND_SECRETS_FILE` can select another file, which is useful for testing or
managed installations. `ASCEND_CONOPT_LICENSE` has precedence over any file.
If neither source supplies a license, ASCEND does not call the registration
function and CONOPT retains its normal unlicensed/demo behavior.

## GitHub Actions

Store the complete four-field value in one repository or organization secret
named `ASCEND_CONOPT_LICENSE`, then expose it only to the trusted test step:

```yaml
- name: Licensed CONOPT tests
  if: github.event_name == 'push' && github.ref_name == github.event.repository.default_branch
  env:
    ASCEND_CONOPT_LICENSE: ${{ secrets.ASCEND_CONOPT_LICENSE }}
  run: ./a4 cutest solver_conopt
```

Do not expose the secret to pull-request jobs that execute untrusted changes.
The implementation never writes or logs the configured license value.
