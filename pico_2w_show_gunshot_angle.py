from machine import UART, Pin
import time
import tft_config
import st7789py as st7789
from math import sin, cos, radians, isnan
import gc

# --- UART Setup ---
uart_map = {
    "esp1": UART(1, baudrate=115200, tx=Pin(4), rx=Pin(5)),    # GP5
    "esp2": UART(0, baudrate=115200, tx=Pin(0), rx=Pin(1)),    # GP1
}

# --- Directional Offsets (180° field each) ---
esp_offsets = {
    "esp1": 90,    # faces right, handles 0° to 180°
    "esp2": 270,   # faces left, handles 180° to 360°
}

readings = {}

# --- TFT Setup ---
tft = tft_config.config(rotation=1)
try:
    tft.backlight.value(1)
except:
    Pin(6, Pin.OUT).value(1)

tft.fill(st7789.BLACK)

# --- Drawing Utilities ---
def select_strongest_direction(readings_dict):
    if not readings_dict:
        return None
    return max(readings_dict.values(), key=lambda x: x[1])[0]

def arrow_color_for_angle(angle):
    angle = angle % 360
    if abs(angle - 0) < 5 or abs(angle - 360) < 5:
        return st7789.RED
    elif abs(angle - 90) < 5:
        return st7789.GREEN
    elif abs(angle - 180) < 5:
        return st7789.BLUE
    elif abs(angle - 270) < 5:
        return st7789.YELLOW
    return st7789.WHITE

def draw_arrow(tft, angle_deg, color):
    cx = tft.width // 2
    cy = tft.height // 2
    angle_rad = radians(angle_deg)
    shaft_len = int(min(tft.width, tft.height) // 2 - 10)
    head_len = 15
    head_width = 12
    shaft_end_x = cx + int((shaft_len - head_len) * sin(angle_rad))
    shaft_end_y = cy - int((shaft_len - head_len) * cos(angle_rad))
    tip_x = cx + int(shaft_len * sin(angle_rad))
    tip_y = cy - int(shaft_len * cos(angle_rad))
    tft.line(cx, cy, shaft_end_x, shaft_end_y, color)
    base_x = shaft_end_x
    base_y = shaft_end_y
    left_x = base_x + int(head_width * cos(angle_rad))
    left_y = base_y + int(head_width * sin(angle_rad))
    right_x = base_x - int(head_width * cos(angle_rad))
    right_y = base_y - int(head_width * sin(angle_rad))
    tft.polygon([(tip_x, tip_y), (left_x, left_y), (right_x, right_y), (tip_x, tip_y)], 0, 0, color)

def draw_ticks_on_rect_edge(tft):
    cx = tft.width // 2
    cy = tft.height // 2
    tick_len = 8
    for angle_deg in range(0, 360, 30):
        angle = radians(angle_deg)
        r = 1.0
        while True:
            x = int(cx + r * sin(angle))
            y = int(cy - r * cos(angle))
            if x <= 0 or x >= tft.width - 1 or y <= 0 or y >= tft.height - 1:
                x1, y1 = x, y
                x0 = int(x1 - tick_len * sin(angle))
                y0 = int(y1 + tick_len * cos(angle))
                color = st7789.RED if angle_deg == 0 else st7789.WHITE
                tft.line(x0, y0, x1, y1, color)
                break
            r += 0.5

# --- Initial Draw ---
draw_ticks_on_rect_edge(tft)
last_drawn = None

# --- Main Loop ---
while True:
    for esp_id, uart in uart_map.items():
        if uart.any():
            try:
                line = uart.readline()
                if line:
                    msg = line.decode().strip()
                    if not msg.startswith("esp"):
                        continue
                    print("Received:", msg)
                    parts = msg.split(",")
                    if len(parts) == 3:
                        esp, angle, db = [p.strip() for p in parts]
                        angle = float(angle)
                        db = float(db)
                        if isnan(angle) or isnan(db):
                            continue  # Skip bad values
                        if esp in esp_offsets:
                            global_angle = (esp_offsets[esp] + angle) % 360
                            readings[esp] = (global_angle, db)
            except Exception as e:
                print("Error:", e)

    best_angle = select_strongest_direction(readings)
    if best_angle is not None and best_angle != last_drawn:
        tft.fill(st7789.BLACK)
        draw_ticks_on_rect_edge(tft)
        draw_arrow(tft, best_angle, arrow_color_for_angle(best_angle))
        last_drawn = best_angle

    gc.collect()
    time.sleep(0.05)