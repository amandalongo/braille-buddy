import asyncio
import random
import sys
from BrailleListener import BrailleListener, BRAILLE_MAP
from speak import speak
import time

# Reverse mapping to look up binary patterns from characters
REVERSE_BRAILLE_MAP = {v: k for k, v in BRAILLE_MAP.items()}

# Define the 5-letter unit structure
ALPHABET_UNITS = [
    {"lesson": 1, "chars": ["A", "B", "C", "D", "E"]},
    {"lesson": 2, "chars": ["F", "G", "H", "I", "J"]},
    {"lesson": 3, "chars": ["K", "L", "M", "N", "O"]},
    {"lesson": 4, "chars": ["P", "Q", "R", "S", "T"]},
    {"lesson": 5, "chars": ["U", "V", "W", "X", "Y", "Z"]},
]

DOT_DIRECTIONS = {
    0: "top left tile",
    1: "middle left tile",
    2: "bottom left tile",
    3: "top right tile",
    4: "middle right tile",
    5: "bottom right tile",
}


def format_braille_positions(binary_str: str) -> str:
    """Converts a 6-bit binary string into spatial verbal directions."""
    if len(binary_str) != 6:
        return "no tiles"

    active_positions = [
        DOT_DIRECTIONS[i] for i, bit in enumerate(binary_str) if bit == "1"
    ]

    if not active_positions:
        return "no tiles"
    elif len(active_positions) == 1:
        return f"place dot on {active_positions[0]}. Click the button when done."
    elif len(active_positions) == 2:
        return f"place dots on {active_positions[0]} and {active_positions[1]}.Click the button when done. "
    else:
        all_but_last = ", ".join(active_positions[:-1])
        return f"place dots on {all_but_last}, and {active_positions[-1]}. Click the button when done."


class UnitQuizEngine:
    def __init__(self, listener: BrailleListener):
        self.listener = listener
        self.total_correct = 0
        self.total_attempted = 0

    async def speak_and_print(self, text: str):
        """Prints formatted text to console and speaks clean text aloud via Ana."""
        print(text)
        speech_text = text.replace("->", "").replace("=", "").replace("-", "").strip()
        if speech_text:
            await speak(speech_text)

    async def teach_lesson(self, lesson_num: int, chars: list[str]):
        """Instructional Phase: Forces retry loops until each character is formed correctly."""
        await self.speak_and_print(
            "Hi, I'm Dotty! I will be your braille learning buddy"
        )
        time.sleep(.5)
        await self.speak_and_print(
            "What is your name?"
        )
        time.sleep(3)
        await self.speak_and_print(
            "Braille buddy device paired!"
        )
        await self.speak_and_print(
            f"\n{'=' * 55}\n Let's start learning! We are going to learn (Letters {chars[0]} through {chars[-1]}) \n{'=' * 55}"
        )

        for letter in chars:
            binary_pattern = REVERSE_BRAILLE_MAP.get(letter, "000000")
            directions = format_braille_positions(binary_pattern)

            # Mastery loop: keeps looping on the same letter until correct
            while True:
                prompt_msg = f"Letter {letter}: {directions}."
                await self.speak_and_print(f"\n-> {prompt_msg}")

                received = await self.listener.wait_for_input()
                if received == letter:
                    await self.speak_and_print(f"Good Job! You made letter {letter}.")
                    break
                else:
                    await self.speak_and_print(f"Oops! you made {received}. Let's try again.")

    async def quiz_lesson(self, lesson_num: int, chars: list[str]):
        """Quiz Phase: Prompts letters out of order and provides audio corrections."""
        await self.speak_and_print("Good Job Amanda! You finished Lesson 1 let's practice what you learned")

        quiz_queue = chars.copy()
        random.shuffle(quiz_queue)

        for target in quiz_queue:
            self.total_attempted += 1
            expected_pattern = REVERSE_BRAILLE_MAP.get(target, "000000")
            directions = format_braille_positions(expected_pattern)

            await self.speak_and_print(f"\n Make character: {target}")

            received = await self.listener.wait_for_input()

            if received == target:
                self.total_correct += 1
                await self.speak_and_print(f"Good Job! You made Letter {received}.")
            elif received == "UNKNOWN":
                await self.speak_and_print(f"Oops, that's not a letter. To form {target}, {directions}.")
            else:
                await self.speak_and_print(f"Oops you made {received}.")

    async def start(self):
        #await self.speak_and_print("Welcome to Braille Buddy, Unit 1 Alphabet Curriculum.")
        #await self.speak_and_print("Progress through 5 lessons covering A through Z.")

        try:
            for unit in ALPHABET_UNITS:
                lesson_num = unit["lesson"]
                chars = unit["chars"]

                await self.teach_lesson(lesson_num, chars)
                await self.quiz_lesson(lesson_num, chars)

                await self.speak_and_print(f"\nLesson {lesson_num} Complete!")

                if lesson_num < len(ALPHABET_UNITS):
                    await self.speak_and_print("Press Enter to move to the next lesson.")
                    loop = asyncio.get_running_loop()
                    await loop.run_in_executor(None, input)

            await self.speak_and_print("\nCongratulations! You have completed Unit 1, Alphabet A through Z!")

        except (asyncio.CancelledError, KeyboardInterrupt):
            await self.speak_and_print("\nUnit progression paused by user.")




async def main():
    listener = BrailleListener()
    if not await listener.connect():
        sys.exit(1)

    engine = UnitQuizEngine(listener)
    try:
        await engine.start()
    finally:
        await listener.disconnect()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nExiting program.")