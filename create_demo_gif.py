#!/usr/bin/env python3
"""
Create an animated GIF demo of the HomeHUB dashboard
Uses Selenium to capture screenshots and PIL to create the animation
"""

import time
import os
from selenium import webdriver
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.common.by import By
from PIL import Image
import io

# Configuration
DEMO_URL = "http://localhost:5000"
OUTPUT_FILE = "HomeHUB_demo.gif"
SCREEN_WIDTH = 1024
SCREEN_HEIGHT = 600
FRAME_DURATION = 800  # milliseconds per frame

def create_demo():
    # Setup Chrome in headless mode with specific window size
    options = Options()
    options.add_argument(f'--window-size={SCREEN_WIDTH},{SCREEN_HEIGHT}')
    options.add_argument('--force-device-scale-factor=1')
    
    driver = webdriver.Chrome(options=options)
    driver.set_window_size(SCREEN_WIDTH, SCREEN_HEIGHT)
    
    frames = []
    
    try:
        print("Starting demo recording...")
        
        # Frame 1-3: Main dashboard
        print("  Capturing main dashboard...")
        driver.get(DEMO_URL)
        # Zoom out to 80% so all tiles fit on screen
        driver.execute_script("document.body.style.zoom = '0.8'")
        time.sleep(2)
        for _ in range(3):
            frames.append(capture_frame(driver))
            time.sleep(0.3)
        
        # Frame 4-6: Click on Bedroom
        print("  Capturing Bedroom details...")
        bedroom_link = driver.find_element(By.CSS_SELECTOR, 'a[href="/room/Bedroom"]')
        bedroom_link.click()
        time.sleep(1)
        for _ in range(4):
            frames.append(capture_frame(driver))
            time.sleep(0.3)
        
        # Frame 7-8: Back to dashboard
        print("  Returning to dashboard...")
        back_btn = driver.find_element(By.CSS_SELECTOR, '.back-btn')
        back_btn.click()
        time.sleep(1)
        for _ in range(2):
            frames.append(capture_frame(driver))
            time.sleep(0.3)
        
        # Frame 9-12: Click on Living Room
        print("  Capturing Living Room details...")
        living_link = driver.find_element(By.CSS_SELECTOR, 'a[href="/room/Living%20Room"]')
        living_link.click()
        time.sleep(1)
        for _ in range(4):
            frames.append(capture_frame(driver))
            time.sleep(0.3)
        
        # Frame 13-15: Back to dashboard
        print("  Final dashboard view...")
        back_btn = driver.find_element(By.CSS_SELECTOR, '.back-btn')
        back_btn.click()
        time.sleep(1)
        for _ in range(3):
            frames.append(capture_frame(driver))
            time.sleep(0.3)
        
        # Save as animated GIF
        print(f"\nSaving {len(frames)} frames to {OUTPUT_FILE}...")
        frames[0].save(
            OUTPUT_FILE,
            save_all=True,
            append_images=frames[1:],
            duration=FRAME_DURATION,
            loop=0
        )
        print(f"✓ Demo saved to {OUTPUT_FILE}")
        
    finally:
        driver.quit()

def capture_frame(driver):
    """Capture current browser view as PIL Image"""
    png_data = driver.get_screenshot_as_png()
    return Image.open(io.BytesIO(png_data))

if __name__ == '__main__':
    create_demo()
