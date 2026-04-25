import pytest
import openai
import os
import httpx

@pytest.fixture
def client():
    return openai.OpenAI(
        base_url="http://localhost:8080/v1",
        api_key="sk-no-key-required"
    )

@pytest.fixture
def raw_client():
    return httpx.Client(base_url="http://localhost:8080/v1")

@pytest.fixture
def model_name():
    return "gemma-4-E2B-it.litertlm"
