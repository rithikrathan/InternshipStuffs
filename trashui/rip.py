from playwright.sync_api import sync_playwright
import os
import time

HTML_FILE = f"file://{os.path.abspath('index.html')}"

# DGUS Output Directories
DWIN_DIR = "DWIN_SET"
ICON_DIR = f"{DWIN_DIR}/Icon_Sources"

CONTROL_NAMES = {
    1: "Light_1",
    2: "Fan_1",
    3: "Light_2",
    4: "Fan_2",
    5: "Light_3",
    6: "Fan_3",
    7: "Television",
    8: "Heater"
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
        
        # CRITICAL FOR DGUS: device_scale_factor=1 prevents OS display scaling (like Retina/HiDPI)
        # This guarantees the background is EXACTLY 480x800 pixels.
        context = browser.new_context(
            viewport={"width": 480, "height": 800},
            device_scale_factor=1 
        )
        
        page = context.new_page()
        page.goto(HTML_FILE)
        
        # Wait for fonts (like JetBrains Mono and FontAwesome) to fully render
        page.wait_for_load_state("networkidle")
        time.sleep(1) 

        # ---------------------------------------------------------
        # 1. CAPTURE FULL BACKGROUND (Base State)
        # ---------------------------------------------------------
        print("📸 Capturing 480x800 Background...")
        main_screen = page.locator("#main-screen")
        main_screen.screenshot(path=f"{DWIN_DIR}/00_MainScreen.png") 

        # ---------------------------------------------------------
        # 2. CAPTURE STATIC WIDGETS
        # ---------------------------------------------------------
        print("📸 Capturing Top Widgets...")
        page.locator("#ui-header").screenshot(path=f"{ICON_DIR}/001_Header.png", omit_background=True)
        page.locator("#widget-temp").screenshot(path=f"{ICON_DIR}/002_Temp_Widget.png", omit_background=True)
        page.locator("#conn-status").screenshot(path=f"{ICON_DIR}/003_Wifi_Connected.png", omit_background=True)

        # Trigger JS to show Disconnected state
        page.evaluate("toggleConnection()")
        time.sleep(0.2)
        page.locator("#conn-status").screenshot(path=f"{ICON_DIR}/004_Wifi_Disconnected.png", omit_background=True)

        # ---------------------------------------------------------
        # 3. CAPTURE CONTROL CARDS (ON / OFF)
        # ---------------------------------------------------------
        print("📸 Capturing Control Modules...")
        icon_id = 10 # Start numbering at 10 for DGUS neatness

        for i in range(1, 9):
            card = page.locator(f"#ctrl-{i}")
            name = CONTROL_NAMES[i]
            
            # Save OFF State
            off_path = f"{ICON_DIR}/{icon_id:03d}_{name}_OFF.png"
            card.screenshot(path=off_path, omit_background=True)
            print(f"  -> Saved {off_path}")
            icon_id += 1
            
            # Click hidden checkbox to trigger CSS Active state
            page.evaluate(f"document.querySelector('#ctrl-{i} input[type=checkbox]').checked = true; document.querySelector('#ctrl-{i} input[type=checkbox]').dispatchEvent(new Event('change'));")
            
            # Wait for the CSS slider animation to finish
            time.sleep(0.3) 
            
            # Save ON State
            on_path = f"{ICON_DIR}/{icon_id:03d}_{name}_ON.png"
            card.screenshot(path=on_path, omit_background=True)
            print(f"  -> Saved {on_path}")
            icon_id += 1
            
            # Reset
            page.evaluate(f"document.querySelector('#ctrl-{i} input[type=checkbox]').checked = false; document.querySelector('#ctrl-{i} input[type=checkbox]').dispatchEvent(new Event('change'));")

        print("\n✅ Extraction Complete! Check the 'DWIN_SET' folder.")
        browser.close()

if __name__ == "__main__":
    rip_assets()
