import importlib.util
import tempfile
from pathlib import Path
import unittest


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "promote_release3_requirements.py"


def load_module():
    spec = importlib.util.spec_from_file_location("promote_release3_requirements", SCRIPT_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class RequirementStatusPromotionTests(unittest.TestCase):
    def setUp(self):
        self.module = load_module()

    def test_promotion_requires_evidence_and_updates_status(self):
        doc = """
## AP-R3-CORE-006 - Result creation

- Status: Approved
- Verification: Unit test `AP_R3_CORE_006_ResultCreationAndEmplacement`.
- Unit Tests: `AP_R3_CORE_006_ResultCreationAndEmplacement.ConvertibleErrorRequiresExplicitConstruction`; `AP_R3_CORE_006_ResultCreationAndEmplacement.DirectConstructionSelectsValueOrError`
"""

        evidence = {
            "AP-R3-CORE-006": {
                "status": "pass",
                "required_checks": [
                    "AP_R3_CORE_006_ResultCreationAndEmplacement",
                    "AP_R3_CORE_006_ResultCreationAndEmplacement.ConvertibleErrorRequiresExplicitConstruction",
                    "AP_R3_CORE_006_ResultCreationAndEmplacement.DirectConstructionSelectsValueOrError",
                ],
            }
        }

        result = self.module.evaluate_requirement(doc, evidence)
        self.assertEqual(result["new_status"], "Implemented")
        self.assertIn("- Status: Implemented", result["updated_text"])

    def test_missing_evidence_keeps_requirement_approved(self):
        doc = """
## AP-R3-LOG-003 - Logger creation and ownership

- Status: Approved
- Verification: Unit test `AP_R3_LOG_003_CreateLoggerOwnsLogger`.
- Unit Tests: `AP_R3_LOG_003_CreateLoggerOwnsLogger.OwnsLoggerOnConstruction`
"""

        result = self.module.evaluate_requirement(doc, {})
        self.assertEqual(result["new_status"], "Approved")
        self.assertIn("- Status: Approved", result["updated_text"])
        self.assertIn("reason", result)

    def test_promotion_ignores_non_approved_entries(self):
        doc = """
## AP-R3-LOG-001 - LogLevel

- Status: Implemented
- Verification: Unit test `AP_R3_LOG_001_LogLevelsHaveExpectedValues`.
"""

        result = self.module.evaluate_requirement(doc, {"AP-R3-LOG-001": {"status": "pass"}})
        self.assertEqual(result["new_status"], "Implemented")
        self.assertEqual(result["updated_text"], doc)

    def test_repeated_promotion_of_same_document_is_idempotent(self):
        doc = """
## AP-R3-CORE-006 - Result creation

- Status: Approved
- Verification: Unit test `AP_R3_CORE_006_ResultCreationAndEmplacement`.
"""

        evidence = {
            "AP-R3-CORE-006": {
                "status": "pass",
                "required_checks": [
                    "AP_R3_CORE_006_ResultCreationAndEmplacement",
                ],
            }
        }

        first = self.module.evaluate_requirement(doc, evidence)
        second = self.module.evaluate_requirement(first["updated_text"], evidence)
        self.assertEqual(second["new_status"], "Implemented")
        self.assertIn("- Status: Implemented", second["updated_text"])


if __name__ == "__main__":
    unittest.main()
