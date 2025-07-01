# Example Python script
import whisper

model = whisper.load_model("base")
result = model.transcribe("your_audio_file.mp3")
print(result["text"])