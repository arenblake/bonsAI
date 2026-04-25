import pytest
import base64
import os

def test_vision_basic(client, model_name):
    """Verify vision input (image_url)."""
    image_path = "tests/assets/dog.jpeg"
    if not os.path.exists(image_path):
        pytest.skip(f"Asset {image_path} not found")

    with open(image_path, "rb") as f:
        img_b64 = base64.b64encode(f.read()).decode("utf-8")
    
    response = client.chat.completions.create(
        model=model_name,
        messages=[
            {
                "role": "user",
                "content": [
                    {"type": "text", "text": "What animal is in this image?"},
                    {"type": "image_url", "image_url": {"url": f"data:image/jpeg;base64,{img_b64}"}}
                ]
            }
        ]
    )
    assert len(response.choices) > 0
    content = response.choices[0].message.content.lower()
    print(f"Vision Response: {content}")
    assert any(x in content for x in ["dog", "animal", "pet"])

def test_audio_basic(client, model_name):
    """Verify audio input (input_audio)."""
    audio_path = "tests/assets/briar-bat-seagull.ogg"
    if not os.path.exists(audio_path):
        pytest.skip(f"Asset {audio_path} not found")

    # Convert to WAV for better compatibility
    import subprocess
    wav_path = "tests/assets/temp.wav"
    subprocess.run(["ffmpeg", "-y", "-i", audio_path, "-ar", "16000", "-ac", "1", wav_path], 
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)

    with open(wav_path, "rb") as f:
        audio_b64 = base64.b64encode(f.read()).decode("utf-8")
    
    response = client.chat.completions.create(
        model=model_name,
        messages=[
            {
                "role": "user",
                "content": [
                    {"type": "text", "text": "Give a brief summary of this audio clip."},
                    {"type": "input_audio", "input_audio": {"data": audio_b64, "format": "wav"}}
                ]
            }
        ]
    )
    assert len(response.choices) > 0
    content = response.choices[0].message.content.lower()
    print(f"Audio Response: {content}")
    assert any(x in content for x in ["sound", "audio", "bramble", "seagull", "bat"])
    
    if os.path.exists(wav_path):
        os.remove(wav_path)
