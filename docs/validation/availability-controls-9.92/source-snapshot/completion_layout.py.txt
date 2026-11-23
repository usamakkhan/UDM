"""Fresh isolated catalog locations for same-volume and mixed-volume controls."""
from pathlib import Path


def validate_catalog_root(work, requested):
    if requested is None:
        return None
    work, root = Path(work).resolve(), Path(requested).resolve()
    if root.exists():
        raise ValueError('Catalog root must be a fresh isolated directory; existing profiles are refused')
    if root == work or root.is_relative_to(work) or work.is_relative_to(root):
        raise ValueError('Separate catalog and output roots must not overlap')
    return root


def catalog_directory(work, root, case, client, trial):
    if case not in ('single-4m', 'single-64m', 'split-64m') or client not in ('udm', 'baseline', 'idm') or not isinstance(trial, int) or trial < 1:
        raise ValueError('Invalid owned comparison identity')
    name = f'{case}-{client}-{trial}'
    return Path(work) / name / 'catalog' if root is None else Path(root) / name / 'catalog'
