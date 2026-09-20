import asyncio
from bleak import BleakScanner, BleakClient

# ESP32 BLE Configuration
CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

# Standard English Braille Lookup Table ("d1d2d3d4d5d6")
BRAILLE_MAP = {
    # Alphabet (A-Z)
    "100000": "A", "110000": "B", "100100": "C", "100110": "D", "100010": "E",
    "110100": "F", "110110": "G", "110010": "H", "010100": "I", "010110": "J",
    "101000": "K", "111000": "L", "101100": "M", "101110": "N", "101010": "O",
    "111100": "P", "111110": "Q", "111010": "R", "011100": "S", "011110": "T",
    "101001": "U", "111001": "V", "010111": "W", "101101": "X", "101111": "Y",
    "101011": "Z",

    # Single-Cell Punctuation & Special Symbols
    "010000": ",",          # Comma (Dot 2)
    "011000": ";",          # Semicolon (Dots 2-3)
    "010010": ":",          # Colon (Dots 2-5)
    "010011": ".",          # Period / Full Stop (Dots 2-5-6)
    "011001": "?",          # Question Mark (Dots 2-3-6)
    "011010": "!",          # Exclamation Mark (Dots 2-3-5)
}

def decode_braille(binary_str: str) -> str:
    """Converts 6-character binary string into an uppercase letter."""
    return BRAILLE_MAP.get(binary_str, "Unknown Pattern / Not A-Z")

def notification_handler(sender, data: bytearray):
    """Callback triggered whenever the ESP32 submit button is pressed."""
    raw_payload = data.decode("utf-8").strip()
    letter = decode_braille(raw_payload)

    print(f"Received Data: '{raw_payload}'")
    print(f" Decoded Letter:  {letter}")

async def main():
    print("Checking for braillebuddy")
    
    device = await BleakScanner.find_device_by_filter(
        lambda d, ad: d.name == "BrailleBuddy"
    )

    if not device:
        print("BrailleBuddy not found")
        return

    print(f"Connecting  at {device.address}...")

    async with BleakClient(device) as client:
        print("Connected successfully!")
        
        # Subscribe to notifications from the ESP32 characteristic
        await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
        print("Listening for press")

        # Keep script running to handle incoming BLE notifications
        while True:
            await asyncio.sleep(1)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nDisconnected and exiting cleanly.")