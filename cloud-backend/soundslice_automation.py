import os
import time
import re
from playwright.sync_api import sync_playwright

def upload_sheet_music(email, password, image_path):
    """
    Automates SoundSlice login and image upload using Playwright.
    Returns the created Slice ID (scorehash).
    """
    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        context = browser.new_context()
        page = context.new_page()

        try:
            print("Step 1: Logging into SoundSlice...")
            page.goto("https://www.soundslice.com/login/")
            
            # Use robust locators
            page.get_by_label("Your email").fill(email)
            page.get_by_label("Password").fill(password)
            page.get_by_role("button", name="Log in").click()
            
            # Wait for login to complete (check for dashboard or nav element)
            # The site-nav-user class is usually present on the user avatar/menu
            try:
                page.wait_for_selector('.site-nav-user', timeout=15000)
                print("Login successful.")
            except:
                # Fallback: check if we are redirected to /manage/ or /feed/
                if "/login/" not in page.url:
                    print(f"Login likely successful, URL is now: {page.url}")
                else:
                    raise Exception("Login failed - still on login page")

            print("Step 2: Navigating directly to Upload Page...")
            page.goto("https://www.soundslice.com/manage/image-upload/")
            page.wait_for_load_state("networkidle")
            
            # Wait for "still processing" banner to disappear (if present)
            processing_banner = page.locator('p.callout:has-text("previous upload is still processing")')
            if processing_banner.count() > 0:
                print("Previous upload still processing. Waiting for it to complete...")
                for wait_attempt in range(60):  # Wait up to 2 minutes
                    if processing_banner.count() == 0:
                        print("Processing complete. Proceeding with upload.")
                        break
                    time.sleep(2)
                    page.reload()
                    page.wait_for_load_state("networkidle")
                else:
                    raise Exception("Timeout waiting for previous upload to finish processing.")
            
            print("Waiting for file input...")
            page.wait_for_selector("#upload", state="attached", timeout=10000)
            
            # Use absolute path for reliability
            absolute_image_path = os.path.abspath(image_path)
            print(f"Setting input files with: {absolute_image_path}")
            page.set_input_files("#upload", absolute_image_path)
            
            print("File selected. Waiting for preview modal (#imagepreview)...")
            try:
                # SoundSlice has a two-step upload flow:
                # 1. After file selection, the #imagepreview div appears with an "Upload image" button
                # 2. After clicking that, the main upload button (#submitupload2) becomes active
                
                # Wait for the modal container itself first
                modal = page.locator('#imagepreview')
                modal.wait_for(state="visible", timeout=30000)
                print("Preview modal appeared.")
                
                # Step 1: Click "Upload image" in the preview modal
                print("Looking for 'Upload image' button in preview modal...")
                
                # The button is inside #imagepreview and has class standard-button
                upload_image_btn = modal.locator('button.standard-button').first
                upload_image_btn.wait_for(state="visible", timeout=10000)
                
                print("Clicking 'Upload image' to confirm the preview...")
                upload_image_btn.click()
                
                # Wait for the modal to disappear or for a redirect to start
                print("Waiting for modal to close or redirect to start...")
                try:
                    # If we are redirecting, this might timeout, which is actually fine
                    upload_image_btn.wait_for(state="hidden", timeout=5000)
                    print(f"Preview modal closed. Current URL: {page.url}")
                except:
                    print(f"Wait for modal hidden timed out. Current URL: {page.url}")
                
                # Check if we've already been redirected or the upload started
                if "/manage/image-upload/" not in page.url:
                    print(f"URL changed to {page.url}. Proceeding to processing wait.")
                else:
                    # Step 2: Handle the final upload button if still on the page
                    print("Still on upload page. Checking for final 'Upload' button...")
                    upload_btn = page.locator('#submitupload2')
                    
                    # If the button is visible and enabled, click it. 
                    # If it's disabled, wait for it to be ready.
                    for _ in range(30): # 30 seconds polling
                        if "/manage/image-upload/" not in page.url:
                            print(f"URL changed during wait. Proceeding.")
                            break
                            
                        if upload_btn.count() > 0:
                            btn_text = upload_btn.inner_text()
                            is_disabled = upload_btn.get_attribute("disabled") is not None
                            
                            if not is_disabled and "0 selected" not in btn_text:
                                print(f"Final upload button ready ('{btn_text}'). Clicking...")
                                upload_btn.click()
                                break
                        time.sleep(1)
                    else:
                        print("Final upload button did not become ready or was already handled.")
                
            except Exception as e:
                print(f"Upload flow failed: {e}")
                # Debug: capture status
                try:
                    print(f"Final URL: {page.url}")
                except: pass
                raise
            
            print(f"Upload initiated (or in progress). Waiting for processing and redirect...")
            
            # SoundSlice redirects to the overview page after upload
            try:
                # Wait up to 2 minutes for processing
                page.wait_for_url(re.compile(r"https://www.soundslice.com/(manage/)?($|\?|#)"), timeout=120000)
                page.wait_for_load_state("networkidle")
                print(f"Redirected to: {page.url}")
            except:
                print(f"Timeout waiting for redirect. Current URL: {page.url}")
                if "/slices/" in page.url or "/edit/" in page.url:
                    print("Directly on a slice page.")
                else:
                    raise
                
            current_url = page.url
            print(f"Current URL: {current_url}")
            
            # Extract scorehash from the newest slice in the list
            # We look for links matching /slices/[scorehash]/
            print("Step 3: Extracting Scorehash from Library...")
            page.wait_for_selector(".sm-scorename", timeout=10000)
            
            # Get all slice links
            links = page.locator("a[href^='/slices/']").all()
            if not links:
                raise Exception("Could not find any slice links in the library.")
                
            # Usually the newest slice is at the top. 
            # We'll take the first link that matches the pattern and isn't a known menu or button
            newest_href = links[0].get_attribute("href")
            print(f"Newest slice link: {newest_href}")
            
            match = re.search(r"/slices/([a-zA-Z0-9]+)/", newest_href)
            if not match:
                raise Exception(f"Could not extract scorehash from link: {newest_href}")
            
            scorehash = match.group(1)
            print(f"Extracted Scorehash: {scorehash}")

            print("Step 4: Verifying Slice Readiness (User flow)...")
            # Navigate to the editor for verification
            page.goto(f"https://www.soundslice.com/slices/{scorehash}/edit/")
            page.wait_for_load_state("networkidle")
            
            try:
                # Look for 'More' button or three-dots menu
                more_menu = page.locator("button:has-text('More'), button[aria-label='More options'], .editor-top-right button").first
                more_menu.wait_for(state="visible", timeout=10000)
                more_menu.click()
                print("Clicked 'three dots' menu.")
                
                # Look for "Export MIDI" to confirm processing is done
                export_midi = page.get_by_text("Export MIDI", exact=False)
                if export_midi.is_visible(timeout=5000):
                    print("'Export MIDI' option is visible. Slice is ready.")
                else:
                    print("Warning: 'Export MIDI' not immediately visible in menu.")
            except Exception as e:
                print(f"Note: Could not perform optional verification step: {e}")
            
            return scorehash

        except Exception as e:
            print(f"Error: {e}")
            page.screenshot(path="error_screenshot.png")
            print("Screenshot saved to error_screenshot.png")
            return None
        finally:
            browser.close()

if __name__ == "__main__":
    from dotenv import load_dotenv
    load_dotenv()
    
    email = os.getenv("SOUNDSLICE_EMAIL")
    password = os.getenv("SOUNDSLICE_PASSWORD")
    # Test image
    image_path = "uploads/test_image.jpg" # Dummy
    
    if not email or not password:
        print("Please set SOUNDSLICE_EMAIL and SOUNDSLICE_PASSWORD in .env")
    else:
        upload_sheet_music(email, password, image_path)
