import unittest

from tools.check_fefw_deadline import classify


class DeadlineCheckTests(unittest.TestCase):
    def test_deadline_exit_is_expected(self):
        verdict = classify(124, 25, 15.0)
        self.assertTrue(verdict.ok)
        self.assertEqual(verdict.status, "deadline")

    def test_killed_run_is_an_overrun(self):
        verdict = classify(None, 25, 15.0)
        self.assertFalse(verdict.ok)
        self.assertEqual(verdict.status, "overrun")

    def test_clean_exit_passes_without_exercising_the_deadline(self):
        verdict = classify(0, 25, 15.0)
        self.assertTrue(verdict.ok)
        self.assertEqual(verdict.status, "clean")
        self.assertIn("not exercised", verdict.detail)

    def test_other_exit_is_a_failure(self):
        for code in (1, 3, 4, 3221225477):
            with self.subTest(code=code):
                verdict = classify(code, 25, 15.0)
                self.assertFalse(verdict.ok)
                self.assertEqual(verdict.status, "failed")


if __name__ == "__main__":
    unittest.main()
