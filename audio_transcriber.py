import sounddevice as sd
import numpy as np
import queue
import threading
import json
import sys
import os
import tempfile
import time
import whisper
import webrtcvad
import traceback

try:
    import google.generativeai as genai
    genai.configure(api_key="AIzaSyAFN_LRZ4PvHDyOwucrZmDqbas6rdsToLM")
    gemini_model = genai.GenerativeModel('models/gemini-1.5-flash')
except ImportError:
    gemini_model = None
    print("[ERROR] google-generativeai not installed. Gemini features disabled.")

# List audio devices
def list_devices():
    devices = sd.query_devices()
    return [f"{i}: {d['name']}" for i, d in enumerate(devices)]

# List audio devices and save to JSON
def save_devices_json(filename="audio_devices.json"):
    try:
        devices = sd.query_devices()
        device_list = [{"index": i, "name": d["name"]} for i, d in enumerate(devices)]
        with open(filename, "w", encoding="utf-8") as f:
            json.dump(device_list, f, ensure_ascii=False, indent=2)
        import os
        print(f"Saved {len(device_list)} devices to {os.path.abspath(filename)}")
    except Exception as e:
        print(f"Error saving devices to JSON: {e}", file=sys.stderr)

# Audio capture and transcription
class AudioTranscriber:
    def __init__(self, device_index, model_name="medium"):  # Use medium for better accuracy
        self.device_index = device_index
        self.model = whisper.load_model(model_name)
        self.q = queue.Queue()
        self.running = False
        self.text = ""
        self.transcription_json_file = "transcription.json"
        self.qa_json_file = "qa_transcription.json"
        self.samplerate = 16000
        self.blocksize = 8000
        self.channels = 1
        self.temp_wav = os.path.join(tempfile.gettempdir(), "whisper_temp.wav")
        self.vad = webrtcvad.Vad(2)  # 0-3, 3 is most aggressive
        self.speech_buffer = []
        self.speech_detected = False
        self.silence_count = 0
        self.max_silence_blocks = 3  # End segment after this many silent blocks

    def save_sentence_json(self, text):
        data = []
        if os.path.exists(self.transcription_json_file):
            with open(self.transcription_json_file, "r", encoding="utf-8") as f:
                try:
                    data = json.load(f)
                except Exception:
                    data = []
        entry = {"index": len(data) + 1, "text": text, "timestamp": time.time()}
        data.append(entry)
        with open(self.transcription_json_file, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)

    def save_qa_json(self, question, answer):
        # Ensure the file exists and is a valid JSON array
        if not os.path.exists(self.qa_json_file):
            with open(self.qa_json_file, "w", encoding="utf-8") as f:
                json.dump([], f)
        data = []
        try:
            with open(self.qa_json_file, "r", encoding="utf-8") as f:
                data = json.load(f)
        except Exception:
            data = []
        entry = {"question": question, "answer": answer, "timestamp": time.time()}
        data.append(entry)
        with open(self.qa_json_file, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
        print(f"[QA] Saved Q: {question}\n[QA] Saved A: {answer}\n[QA] Appended to {os.path.abspath(self.qa_json_file)}")

    def get_gemini_answer(self, question):
        if gemini_model is None:
            return "[Gemini not available]"
        try:
            response = gemini_model.generate_content(question)
            if hasattr(response, 'text'):
                return response.text
            elif hasattr(response, 'candidates') and response.candidates:
                return response.candidates[0].text
            else:
                return str(response)
        except Exception as e:
            return f"[Gemini error: {e}]"

    def callback(self, indata, frames, time, status):
        try:
            if status:
                print(status, file=sys.stderr)
            self.q.put(indata.copy())
        except Exception as e:
            print(f"[ERROR] Exception in callback: {e}")
            traceback.print_exc()

    def start(self):
        self.running = True
        threading.Thread(target=self._run, daemon=True).start()

    def stop(self):
        self.running = False

    def is_speech(self, audio_chunk):
        # audio_chunk: numpy array, int16, mono
        pcm = audio_chunk.tobytes()
        return self.vad.is_speech(pcm, self.samplerate)

    def _run(self):
        audio_buffer = []
        last_sentence = None
        vad_frame_ms = 10
        vad_frame_samples = int(self.samplerate * vad_frame_ms / 1000)  # 160 for 10ms at 16kHz
        vad_bytes = vad_frame_samples * 2  # 2 bytes per int16 sample
        vad_buf = np.array([], dtype=np.int16)
        with sd.InputStream(samplerate=self.samplerate, blocksize=self.blocksize, device=self.device_index, dtype='int16', channels=self.channels, callback=self.callback):
            print("Transcribing... Press Ctrl+C to stop.")
            try:
                while self.running:
                    try:
                        data = self.q.get()
                        data = data.flatten()
                        vad_buf = np.concatenate([vad_buf, data])
                        # Process in 10ms frames
                        while len(vad_buf) >= vad_frame_samples:
                            frame = vad_buf[:vad_frame_samples]
                            vad_buf = vad_buf[vad_frame_samples:]
                            try:
                                is_speech = self.vad.is_speech(frame.tobytes(), self.samplerate)
                            except Exception as e:
                                print(f"[VAD ERROR] {e}")
                                is_speech = False
                            if is_speech:
                                self.speech_buffer.append(frame)
                                self.speech_detected = True
                                self.silence_count = 0
                            else:
                                if self.speech_detected:
                                    self.silence_count += 1
                                    if self.silence_count >= self.max_silence_blocks:
                                        # End of speech segment, process
                                        if len(self.speech_buffer) > 0:
                                            audio_np = np.concatenate(self.speech_buffer, axis=0)
                                            import soundfile as sf
                                            sf.write(self.temp_wav, audio_np, self.samplerate)
                                            result = self.model.transcribe(self.temp_wav, language='en')
                                            sentence = result.get('text', '').strip()
                                            if sentence and (last_sentence is None or sentence != last_sentence) and len(sentence) > 5:
                                                print(f"[Transcribed] {sentence}")
                                                self.text += sentence + '\n'
                                                self.save_sentence_json(sentence)
                                                answer = self.get_gemini_answer(sentence)
                                                print(f"[Gemini Answer] {answer}")
                                                self.save_qa_json(sentence, answer)
                                                last_sentence = sentence
                                            else:
                                                print("[Transcribed] (empty, repeated, or too short)")
                                        self.speech_buffer = []
                                        self.speech_detected = False
                                        self.silence_count = 0
                                # else: not in speech, do nothing
                        # Wait a bit to avoid rapid repeats
                        time.sleep(0.1)
                    except Exception as e:
                        print(f"[ERROR] Error while processing frame: {e}")
                        traceback.print_exc()
            except Exception as e:
                print(f"[ERROR] {e}")
                traceback.print_exc()

    def get_text(self):
        return self.text

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--list-devices', action='store_true')
    parser.add_argument('--list-devices-json', action='store_true')
    parser.add_argument('--device', type=int, default=None)
    args = parser.parse_args()

    if args.list_devices_json:
        save_devices_json()
        sys.exit(0)

    if args.list_devices:
        for d in list_devices():
            print(d)
        sys.exit(0)

    if args.device is not None:
        transcriber = AudioTranscriber(args.device)
        transcriber.start()
        print("Transcribing... Press Ctrl+C to stop.")
        try:
            while True:
                print(transcriber.get_text(), end='\r')
                sd.sleep(1000)
        except KeyboardInterrupt:
            transcriber.stop()
            print("\nStopped.")
    else:
        print("Please specify --device <index> or --list-devices")
