import pytest

def test_list_models(client, model_name):
    """Verify that /v1/models returns the correct model."""
    models = client.models.list()
    ids = [m.id for m in models.data]
    assert model_name in ids
    assert any(m.owned_by == "bonsai" for m in models.data)
