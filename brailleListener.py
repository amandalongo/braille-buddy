import asyncio
from bleak import BleakScanner, BleakClient

CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BRAILLE_MAP = {
    # Alphabet (A-Z)
    "100000": "A", "110000": "B", "100100": "C", "100110": "D", "100010": "E",
    "110100": "F", "110110": "G", "110010": "H", "010100": "I", "010110": "J",
    "101000": "K", "111000": "L", "101100": "M", "101110": "N", "101010": "O",
    "111100": "P", "111110": "Q", "111010": "R", "011100": "S", "011110": "T",
    "101001": "U", "111001": "V", "010111": "W", "101101": "X", "101111": "Y",
    "101011": "Z",

    # Single-Cell Punctuation & Special Symbols
    "010000": ",",
    "011000": ";",  
    "010010": ":",     
    "010011": ".",   
    "011001": "?", 
    "011010": "!",
}

class BrailleListener:
    def __init__(self):
        self.client = None
        self.event = asyncio.Event()
        self.last_letter = ""

    def _notification_handler(self, sender, data: bytearray):
        raw_binary = data.decode("utf-8").strip()
        self.last_letter = BRAILLE_MAP.get(raw_binary, "not a letter")
        self.event.set()

    async def connect(self) -> bool:
        print("Scanning for 'BrailleBuddy' over Bluetooth...")
        device = await BleakScanner.find_device_by_filter(
            lambda d, ad: d.name == "BrailleBuddy"
        )
        if not device:
            print("Could not find BrailleBuddy!")
            return False

        self.client = BleakClient(device)
        await self.client.connect()
        await self.client.start_notify(CHARACTERISTIC_UUID, self._notification_handler)
        print("Connected to BrailleBuddy Hardware!")
        return True

    async def wait_for_input(self) -> str:
        """Blocks until the hardware submit button is pressed, then returns the letter."""
        self.event.clear()
        await self.event.wait()
        return self.last_letter

    async def disconnect(self):
        if self.client and self.client.is_connected:
            await self.client.disconnect()