import tempfile
import unittest
import uuid
from pathlib import Path
from completion_layout import validate_catalog_root, catalog_directory


class CompletionLayout(unittest.TestCase):
    def setUp(self):
        # Read-only path checks: create or delete no test directories.
        self.fresh = Path(tempfile.gettempdir()) / ('udm-layout-' + uuid.uuid4().hex)
        self.work = self.fresh / 'outputs'

    def test_default_keeps_the_existing_catalog_location(self):
        self.assertIsNone(validate_catalog_root(self.work, None))
        self.assertEqual(catalog_directory(self.work, None, 'single-4m', 'udm', 1), self.work / 'single-4m-udm-1' / 'catalog')

    def test_existing_personal_or_other_profile_is_refused(self):
        with self.assertRaisesRegex(ValueError, 'existing profiles are refused'):
            validate_catalog_root(self.work, Path(tempfile.gettempdir()))

    def test_equal_nested_and_ancestor_roots_are_refused(self):
        for root in (self.work, self.work / 'cache', self.fresh):
            with self.assertRaisesRegex(ValueError, 'must not overlap'):
                validate_catalog_root(self.work, root)

    def test_separate_fresh_root_retains_each_clients_unique_catalog(self):
        root = validate_catalog_root(self.work, self.fresh / 'cache')
        paths = {catalog_directory(self.work, root, 'single-4m', client, trial) for client in ('udm', 'baseline') for trial in (1, 2)}
        self.assertEqual(len(paths), 4)
        self.assertTrue(all(path.is_relative_to(root) for path in paths))

    def test_comparison_components_cannot_escape_the_owned_root(self):
        for case, client, trial in [('../outside', 'udm', 1), ('single-4m', '../outside', 1), ('single-4m', 'udm', 0)]:
            with self.assertRaises(ValueError):
                catalog_directory(self.work, self.fresh / 'cache', case, client, trial)


if __name__ == '__main__':
    unittest.main()
