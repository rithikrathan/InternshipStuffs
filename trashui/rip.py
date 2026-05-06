from playwright.sync_api import sync_playwright
import os
import time

HTML_FILE = f"file://{os.path.abspath('index.html')}"

DWIN_DIR = "DWIN_SET"
ICON_DIR = f"{DWIN_DIR}/Icon_Sources"

# Spatial padding so outer glows/shadows are never clipped
PADDING_PX = 0 

CONTROL_NAMES = {
    1: "Light_1", 2: "Fan_1", 3: "Light_2", 4: "Fan_2",
    5: "Light_3", 6: "Fan_3", 7: "Television", 8: "Heater"
}

def setup_directories():
    if not os.path.exists(DWIN_DIR):
        os.makedirs(DWIN_DIR)
    if not os.path.exists(ICON_DIR):
        os.makedirs(ICON_DIR)

def rip_assets():
    setup_directories()

    with sync_playwright() as p:
        print("🚀 Launching browser engine...")
        browser = p.chromium.launch(headless=True)
        
        # Hard-lock to 480x800, bypass OS display scaling
        context = browser.new_context(
            viewport={"width": 480, "height": 800},
            device_scale_factor=1 
        )
        
        page = context.new_page()
        page.goto(HTML_FILE)
        page.wait_for_load_state("networkidle")
        time.sleep(1.5) # Wait for custom fonts to render

        main_screen = page.locator("#main-screen")

        # Custom function to capture elements with extra spatial padding
        def capture_padded(locator_str, filepath):
            element = page.locator(locator_str)
            box = element.bounding_box()
            
            page.screenshot(
                path=filepath,
                clip={
                    "x": box["x"] - PADDING_PX,
                    "y": box["y"] - PADDING_PX,
                    "width": box["width"] + (PADDING_PX * 2),
                    "height": box["height"] + (PADDING_PX * 2)
                },
                omit_background=True
            )

        # ---------------------------------------------------------
        # 1. CAPTURE GUIDE SCREEN (Pure black with Magenta Hitboxes)
        # ---------------------------------------------------------
        print("📸 Generating Hitbox Guide Screen...")
        page.evaluate("window.setDgusGuideMode()")
        time.sleep(0.5)
        main_screen.screenshot(path=f"{DWIN_DIR}/00_Guide_Hitboxes.png") 

        # ---------------------------------------------------------
        # 2. CAPTURE CLEAN BASE BACKGROUND
        # ---------------------------------------------------------
        print("📸 Capturing Clean 480x800 Background...")
        page.evaluate("window.setDgusBaseMode()")
        time.sleep(0.5)
        main_screen.screenshot(path=f"{DWIN_DIR}/00_MainScreen.png") 

        # ---------------------------------------------------------
        # 3. CAPTURE ALL ICONS & WIDGETS WITH PADDING
        # ---------------------------------------------------------
        print("📸 Capturing Top Widgets with Padding...")
        page.evaluate("window.resetDgusMode()") 
        time.sleep(0.5)

        # Header, Temp, and WiFi badges
        capture_padded("#ui-header", f"{ICON_DIR}/001_Header.png")
        capture_padded("#widget-temp", f"{ICON_DIR}/002_Temp_Widget.png")
        capture_padded("#conn-status", f"{ICON_DIR}/003_Wifi_Connected.png")

        # Trigger JS to show Disconnected state
        page.evaluate("toggleConnection()")
        time.sleep(0.3)
        capture_padded("#conn-status", f"{ICON_DIR}/004_Wifi_Disconnected.png")
        page.evaluate("toggleConnection()") # reset

        print("📸 Capturing Control Modules with Padding...")
        icon_id = 10 

        for i in range(1, 9):
            card_id = f"#ctrl-{i}"
            name = CONTROL_NAMES[i]
            
            # Save Padded OFF State
            capture_padded(card_id, f"{ICON_DIR}/{icon_id:03d}_{name}_OFF.png")
            print(f"  -> Saved {icon_id:03d}_{name}_OFF.png")
            icon_id += 1
            
            # [FIXED HERE] Click the visible slider instead of the invisible checkbox
            toggle_slider = page.locator(f"{card_id} .slider")
            toggle_slider.click(force=True)
            
            time.sleep(0.4) # Wait for slide animation
            
            # Save Padded ON State
            capture_padded(card_id, f"{ICON_DIR}/{icon_id:03d}_{name}_ON.png")
            print(f"  -> Saved {icon_id:03d}_{name}_ON.png")
            icon_id += 1
            
            # Reset
            toggle_slider.click(force=True)
            time.sleep(0.2)

        print(f"\n✅ Extraction Complete! Check the '{DWIN_DIR}' folder.")
        browser.close()

if __name__ == "__main__":
    rip_assets()
