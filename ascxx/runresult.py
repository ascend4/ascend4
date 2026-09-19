"""Detached results for ASCEND execution; no native handles or GUI dependencies."""

from dataclasses import asdict, dataclass, field


@dataclass
class RunDiagnostic:
    phase: str
    message: str
    severity: str = "error"
    exception_type: str = ""


@dataclass
class RunResult:
    """Snapshot of one execution, including any output retained on failure.

    Tables contain columns (label, kind, display units) and rows (time in
    display/base units, typed values, event marker). Values hold scalar
    snapshots. Neither collection contains native instance handles.
    """

    action: str
    status: str = "success"
    phase: str = ""
    diagnostics: list = field(default_factory=list)
    tables: list = field(default_factory=list)
    values: dict = field(default_factory=dict)
    simulation_status: dict = field(default_factory=dict)
    notes: list = field(default_factory=list)

    @property
    def ok(self):
        return self.status == "success"

    @property
    def partial(self):
        """Whether a failed run retained rows; not a numerical accuracy claim."""
        return not self.ok and any(table["rows"] for table in self.tables)

    def fail(self, phase, error):
        self.status = "failed"
        self.phase = phase
        self.diagnostics.append(RunDiagnostic(phase, str(error), exception_type=type(error).__name__))

    def as_dict(self):
        """Return a copy made only of Python containers and scalar values."""
        return asdict(self)

    def raise_for_status(self):
        if not self.ok:
            raise RunError(self)


class RunError(RuntimeError):
    """A failed execution with its structured result, including partial output."""

    def __init__(self, result):
        self.result = result
        messages = [d.message for d in result.diagnostics if d.severity == "error"]
        super().__init__("; ".join(messages) or "ASCEND execution failed")
