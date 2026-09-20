import argparse
import asyncio
import os
import edge_tts
import pygame

# Fixed voice selection
VOICE = "en-US-AnaNeural"


async def speak(text: str) -> None:
    """Generates speech using Edge TTS (Ana voice) and plays it asynchronously using Pygame."""
    if not text.strip():
        return

    output_file = "temp_speech.mp3"

    try:
        # Generate speech audio file
        communicate = edge_tts.Communicate(text, VOICE)
        await communicate.save(output_file)

        # Initialize audio playback
        pygame.mixer.init()
        pygame.mixer.music.load(output_file)
        pygame.mixer.music.play()

        # Keep playing until audio finishes
        while pygame.mixer.music.get_busy():
            await asyncio.sleep(0.05)

        pygame.mixer.quit()

    finally:
        # Clean up temporary audio file
        if os.path.exists(output_file):
            try:
                os.remove(output_file)
            except PermissionError:
                pass


async def main():
    parser = argparse.ArgumentParser(description="Text-to-Speech runner using Ana's voice via edge-tts.")
    parser.add_argument("text", type=str, nargs="?", default="Hello! Ready to learn Braille together?", help="Text to speak")
    
    args = parser.parse_args()

    print(f"Speaking with Ana: '{args.text}'")
    await speak(args.text)


if __name__ == "__main__":
    asyncio.run(main())