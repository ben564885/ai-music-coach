import os
import time
import tempfile
import requests
from playwright.sync_api import sync_playwright, expect

def convert_audio_to_midi(audio_path):
    """
    Automates Samplab audio-to-MIDI conversion using Playwright.
    
    Args:
        audio_path: Path to the audio file (WAV, MP3, etc.)
    
    Returns:
        MIDI file content as bytes, or None if conversion failed
    """
    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        context = browser.new_context(
            accept_downloads=True  # Enable downloads
        )
        page = context.new_page()

        try:
            print("Step 1: Navigating to Samplab audio-to-MIDI page...")
            page.goto("https://samplab.com/audio-to-midi")
            page.wait_for_load_state("networkidle")
            
            print("Step 2: Logging into Samplab...")
            # Find and click the login link/button in the top right
            try:
                login_link = page.get_by_text("Log In", exact=False).first
                if login_link.count() == 0:
                    # Try alternative selectors
                    login_link = page.locator('a:has-text("Log In"), button:has-text("Log In")').first
                
                if login_link.count() > 0:
                    login_link.click()
                    page.wait_for_load_state("networkidle")
                    time.sleep(1)  # Brief wait for login form to appear
                    
                    # Fill in email
                    email_input = page.get_by_label("email", exact=False).first
                    if email_input.count() == 0:
                        email_input = page.locator('input[type="email"], input[name*="email"], input[placeholder*="email" i]').first
                    
                    # Get credentials from environment variables
                    email = os.getenv("SAMPLAB_EMAIL")
                    password = os.getenv("SAMPLAB_PASSWORD")
                    
                    if not email or not password:
                        raise Exception("SAMPLAB_EMAIL and SAMPLAB_PASSWORD must be set in environment variables")
                    
                    if email_input.count() > 0:
                        email_input.fill(email)
                        print("Email filled in")
                    else:
                        raise Exception("Could not find email input field")
                    
                    # Fill in password
                    password_input = page.get_by_label("password", exact=False).first
                    if password_input.count() == 0:
                        password_input = page.locator('input[type="password"], input[name*="password"]').first
                    
                    if password_input.count() > 0:
                        password_input.fill(password)
                        print("Password filled in")
                    else:
                        raise Exception("Could not find password input field")
                    
                    # Click submit/login button
                    submit_button = page.get_by_role("button", name="Log in", exact=False).first
                    if submit_button.count() == 0:
                        submit_button = page.get_by_role("button", name="Sign in", exact=False).first
                    if submit_button.count() == 0:
                        submit_button = page.locator('button[type="submit"], button:has-text("Log"), button:has-text("Sign")').first
                    
                    if submit_button.count() > 0:
                        submit_button.click()
                        print("Login button clicked")
                        # Wait for login to complete
                        page.wait_for_load_state("networkidle")
                        time.sleep(3)  # Wait for redirect/navigation after login
                        
                        # Verify login success by checking for "Log out" button
                        logout_button = page.get_by_text("Log out", exact=False).first
                        if logout_button.count() == 0:
                            logout_button = page.get_by_text("Logout", exact=False).first
                        if logout_button.count() == 0:
                            logout_button = page.locator('a:has-text("Log out"), button:has-text("Log out"), a:has-text("Logout"), button:has-text("Logout")').first
                        
                        if logout_button.count() > 0:
                            print("✓ Login SUCCESSFUL - Log out button found in top right corner")
                        else:
                            # Also check if we're still on login page
                            if "/login" in page.url.lower() or "/signin" in page.url.lower():
                                print("✗ Login FAILED - Still on login page and no Log out button found")
                                raise Exception("Login failed - still on login page")
                            else:
                                print("⚠ Login status UNKNOWN - Not on login page but Log out button not found")
                    else:
                        raise Exception("Could not find login submit button")
                else:
                    # Check if already logged in by looking for Log out button
                    logout_button = page.get_by_text("Log out", exact=False).first
                    if logout_button.count() == 0:
                        logout_button = page.get_by_text("Logout", exact=False).first
                    if logout_button.count() > 0:
                        print("✓ Already logged in - Log out button found in top right corner")
                    else:
                        user_menu = page.locator('[class*="user"], [class*="profile"], [class*="account"]').first
                        if user_menu.count() > 0:
                            print("⚠ Already logged in (user menu found, but Log out button not visible)")
                        else:
                            raise Exception("Could not find login link and not already logged in")
            except Exception as login_error:
                print(f"✗ Login error: {login_error}")
                # Final check - look for Log out button to see if login actually succeeded despite error
                try:
                    logout_button = page.get_by_text("Log out", exact=False).first
                    if logout_button.count() == 0:
                        logout_button = page.get_by_text("Logout", exact=False).first
                    if logout_button.count() > 0:
                        print("✓ Login actually SUCCESSFUL - Log out button found despite error message")
                    else:
                        # Check if we're on login page
                        if "/login" in page.url.lower() or "/signin" in page.url.lower():
                            print("✗ Login FAILED - Still on login page")
                            raise login_error  # Re-raise if we're on login page
                        else:
                            print("⚠ Login status UNKNOWN - Not on login page but Log out button not found")
                            raise login_error
                except:
                    raise login_error
            
            print("Step 3: Navigating to audio-to-MIDI page (if not already there)...")
            if "/audio-to-midi" not in page.url:
                page.goto("https://samplab.com/audio-to-midi")
                page.wait_for_load_state("networkidle")
            
            # Wait for the page to be ready
            print("Step 4: Waiting for upload area...")
            page.wait_for_load_state("networkidle")
            
            print("Step 5: Uploading audio file...")
            # Try multiple strategies to find and use the file input
            absolute_audio_path = os.path.abspath(audio_path)
            file_uploaded = False
            
            # Strategy 1: Look for file input directly (might be hidden)
            file_input = page.locator('input[type="file"]')
            if file_input.count() > 0:
                print(f"Found file input, setting file: {absolute_audio_path}")
                file_input.first.set_input_files(absolute_audio_path)
                file_uploaded = True
            else:
                # Strategy 2: Click the upload area (textbox) and wait for file input to appear
                upload_area = page.locator('[role="textbox"]').first
                if upload_area.count() > 0:
                    print("Clicking upload area to trigger file input...")
                    upload_area.click()
                    time.sleep(1)  # Wait for file input to appear if dynamic
                    
                    # Try to find file input again
                    file_input = page.locator('input[type="file"]')
                    if file_input.count() > 0:
                        print(f"File input appeared, setting file: {absolute_audio_path}")
                        file_input.first.set_input_files(absolute_audio_path)
                        file_uploaded = True
                
                # Strategy 3: Try using set_input_files on the upload area container
                if not file_uploaded:
                    # Look for the container div that might accept file drops
                    upload_container = page.locator('text="Drag & drop your audio file here"').locator('..').first
                    if upload_container.count() > 0:
                        try:
                            upload_container.set_input_files(absolute_audio_path)
                            file_uploaded = True
                            print(f"Set file via container: {absolute_audio_path}")
                        except:
                            pass
            
            if not file_uploaded:
                raise Exception("Could not find file input element. Page structure may have changed.")
            
            print("File uploaded. Waiting for processing...")
            time.sleep(3)  # Give it more time for the UI to update with options
            
            # Wait for any toggle/switch elements to appear
            try:
                page.wait_for_load_state("networkidle", timeout=5000)
            except:
                pass
            
            print("Step 6: Untoggling separate stems option...")
            # Take a screenshot for debugging if needed
            debug_screenshot = False
            # Look for the "Separate stems" toggle switch and untoggle it
            # Toggle switches are often implemented as buttons, switches, or styled checkboxes
            try:
                bands_untoggled = False
                
                # Strategy 1: Look for text "Separate stems" and find nearby toggle/switch
                try:
                    separate_stems_text = page.get_by_text("Separate stems", exact=False)
                    if separate_stems_text.count() > 0:
                        # Find toggle/switch near the text - could be a button, switch, or checkbox
                        # Look for common toggle patterns: button[role="switch"], input[type="checkbox"], or toggle div
                        toggle = separate_stems_text.locator('..').locator('button[role="switch"], input[type="checkbox"], button.toggle, [role="switch"]').first
                        if toggle.count() == 0:
                            # Toggle might be a sibling
                            toggle = separate_stems_text.locator('../..').locator('button[role="switch"], input[type="checkbox"], button.toggle, [role="switch"]').first
                        if toggle.count() == 0:
                            # Look for any toggle-like element near the text
                            toggle = separate_stems_text.locator('..').locator('button, [role="switch"], input').first
                        
                        if toggle.count() > 0:
                            # Check if it's toggled on (right position)
                            # For switches, check aria-checked, checked state, or class names
                            is_toggled = False
                            try:
                                aria_checked = toggle.get_attribute('aria-checked')
                                if aria_checked == 'true':
                                    is_toggled = True
                            except:
                                pass
                            
                            if not is_toggled:
                                try:
                                    if toggle.evaluate('el => el.checked') == True:
                                        is_toggled = True
                                except:
                                    pass
                            
                            if not is_toggled:
                                # Check if toggle has "on" or "active" class
                                try:
                                    class_name = toggle.get_attribute('class') or ''
                                    if 'on' in class_name.lower() or 'active' in class_name.lower() or 'checked' in class_name.lower():
                                        is_toggled = True
                                except:
                                    pass
                            
                            if is_toggled:
                                print("Found 'Separate stems' toggle in ON position, clicking to turn OFF...")
                                toggle.click()
                                bands_untoggled = True
                            else:
                                print("Found 'Separate stems' toggle but it's already OFF")
                                # Still mark as handled since it's already OFF (desired state)
                                bands_untoggled = True
                except Exception as e:
                    print(f"Strategy 1 failed: {e}")
                    pass
                
                # Strategy 2: Look for all toggle switches and check their associated text
                if not bands_untoggled:
                    try:
                        # Look for buttons with role="switch" or toggle-like elements
                        toggles = page.locator('button[role="switch"], [role="switch"], input[type="checkbox"]').all()
                        for toggle in toggles:
                            try:
                                # Get nearby text to see if it's the separate stems toggle
                                parent = toggle.locator('..')
                                text_content = parent.inner_text() if parent.count() > 0 else ""
                                if text_content and 'separate' in text_content.lower() and ('stem' in text_content.lower() or 'band' in text_content.lower()):
                                    # Check if it's toggled on
                                    is_toggled = False
                                    try:
                                        aria_checked = toggle.get_attribute('aria-checked')
                                        if aria_checked == 'true':
                                            is_toggled = True
                                    except:
                                        pass
                                    if not is_toggled:
                                        try:
                                            if toggle.evaluate('el => el.checked') == True:
                                                is_toggled = True
                                        except:
                                            pass
                                    
                                    if is_toggled:
                                        print("Found 'Separate stems' toggle by scanning all toggles, clicking to turn OFF...")
                                        toggle.click()
                                        bands_untoggled = True
                                        break
                                    else:
                                        # If found but already OFF, that's fine too
                                        print("Found 'Separate stems' toggle but already OFF")
                                        bands_untoggled = True
                                        break
                            except:
                                continue
                    except Exception as e:
                        print(f"Strategy 2 failed: {e}")
                        pass
                
                # Strategy 3: Look for toggle by finding text and clicking the toggle element next to it
                if not bands_untoggled:
                    try:
                        # Find the text element
                        text_elem = page.get_by_text("Separate stems", exact=False).first
                        if text_elem.count() > 0:
                            # Look for clickable toggle elements in the same container
                            container = text_elem.locator('..')
                            # Try clicking any button or switch in the container
                            toggle_btn = container.locator('button, [role="switch"], input[type="checkbox"]').first
                            if toggle_btn.count() > 0:
                                print("Found toggle near 'Separate stems' text, clicking to ensure it's OFF...")
                                toggle_btn.click()
                                bands_untoggled = True
                    except Exception as e:
                        print(f"Strategy 3 failed: {e}")
                        pass
                
                # Strategy 4: More aggressive search - look for ANY toggle/switch and check nearby text
                if not bands_untoggled:
                    try:
                        print("Trying aggressive search for toggle...")
                        # Get all interactive elements that could be toggles
                        all_toggles = page.locator('button, [role="switch"], input[type="checkbox"], [class*="toggle"], [class*="switch"]').all()
                        print(f"Found {len(all_toggles)} potential toggle elements")
                        
                        for toggle in all_toggles:
                            try:
                                # Get all text within the same container
                                container = toggle.locator('..')
                                text_content = container.inner_text() if container.count() > 0 else ""
                                
                                # Also check parent's parent
                                if not text_content or ('separate' not in text_content.lower() or 'band' not in text_content.lower()):
                                    container = toggle.locator('../..')
                                    text_content = container.inner_text() if container.count() > 0 else ""
                                
                                if text_content and 'separate' in text_content.lower() and ('stem' in text_content.lower() or 'band' in text_content.lower()):
                                    print(f"Found toggle with 'separate stems' text: {text_content[:100]}")
                                    # Check if it's toggled on
                                    is_toggled = False
                                    
                                    # Check various ways to determine if toggle is ON
                                    try:
                                        aria_checked = toggle.get_attribute('aria-checked')
                                        if aria_checked == 'true':
                                            is_toggled = True
                                            print("  - Toggle is ON (aria-checked=true)")
                                    except:
                                        pass
                                    
                                    if not is_toggled:
                                        try:
                                            checked = toggle.evaluate('el => el.checked')
                                            if checked == True:
                                                is_toggled = True
                                                print("  - Toggle is ON (checked=true)")
                                        except:
                                            pass
                                    
                                    if not is_toggled:
                                        try:
                                            class_name = toggle.get_attribute('class') or ''
                                            if any(word in class_name.lower() for word in ['on', 'active', 'checked', 'enabled']):
                                                is_toggled = True
                                                print(f"  - Toggle appears ON (class: {class_name})")
                                        except:
                                            pass
                                    
                                    # If we found the toggle, click it regardless of state to ensure it's OFF
                                    print(f"Clicking 'Separate stems' toggle to ensure it's OFF...")
                                    toggle.click()
                                    bands_untoggled = True
                                    break
                            except Exception as e:
                                continue
                    except Exception as e:
                        print(f"Strategy 4 failed: {e}")
                        pass
                
                # Strategy 5: Look for text "Separate stems" and click any clickable element nearby
                if not bands_untoggled:
                    try:
                        print("Trying to find by text and click nearby element...")
                        text_elem = page.get_by_text("Separate stems", exact=False).first
                        if text_elem.count() > 0:
                            print("Found 'Separate stems' text!")
                            # Get the parent container
                            container = text_elem.locator('..')
                            # Look for any clickable element (button, div with click handler, etc.)
                            clickable = container.locator('button, [role="button"], [role="switch"], input, div[onclick], span[onclick]').first
                            if clickable.count() == 0:
                                # Try parent's parent
                                container = text_elem.locator('../..')
                                clickable = container.locator('button, [role="button"], [role="switch"], input, div[onclick]').first
                            
                            if clickable.count() > 0:
                                print("Found clickable element near 'Separate stems' text, clicking...")
                                clickable.click()
                                bands_untoggled = True
                            else:
                                print("No clickable element found near 'Separate stems' text")
                    except Exception as e:
                        print(f"Strategy 5 failed: {e}")
                        pass
                
                if not bands_untoggled:
                    print("ERROR: 'Separate stems' toggle not found!")
                    print("Taking screenshot for debugging...")
                    page.screenshot(path="samplab_separate_stems_not_found.png")
                    print("Screenshot saved to samplab_separate_stems_not_found.png")
                    # Try to get page HTML to debug
                    try:
                        html_snippet = page.locator('body').inner_html()[:2000]
                        print(f"Page HTML snippet (first 2000 chars): {html_snippet}")
                    except:
                        pass
                    raise Exception("Could not find 'Separate stems' toggle - this is required!")
                
                time.sleep(0.5)  # Brief wait after any changes
            except Exception as e:
                print(f"ERROR: Could not find/untoggle separate stems option: {e}")
                import traceback
                traceback.print_exc()
                # Take screenshot for debugging
                try:
                    page.screenshot(path="samplab_toggle_error.png")
                    print("Error screenshot saved to samplab_toggle_error.png")
                except:
                    pass
                raise  # Re-raise the exception since this is critical
            
            print("Step 7: Clicking 'Convert to MIDI' button...")
            convert_button = page.get_by_role("button", name="Convert to MIDI")
            convert_button.wait_for(state="visible", timeout=10000)
            convert_button.click()
            
            print("Step 8: Waiting for conversion to complete...")
            # Wait for conversion to finish - look for download button or completion indicator
            # The page should show a download button after conversion
            download_button = page.get_by_text("Download MIDI", exact=False)
            download_button.wait_for(state="visible", timeout=120000)  # Wait up to 2 minutes
            
            print("Step 9: Downloading MIDI file...")
            # Set up download listener
            with page.expect_download() as download_info:
                download_button.click()
            
            download = download_info.value
            # Save to temporary file
            temp_midi_path = os.path.join(tempfile.gettempdir(), f"samplab_midi_{int(time.time())}.mid")
            download.save_as(temp_midi_path)
            
            # Read the MIDI file content
            with open(temp_midi_path, 'rb') as f:
                midi_content = f.read()
            
            # Clean up temp file
            os.remove(temp_midi_path)
            
            print(f"MIDI conversion successful. Downloaded {len(midi_content)} bytes.")
            return midi_content

        except Exception as e:
            print(f"Error during Samplab conversion: {e}")
            import traceback
            traceback.print_exc()
            page.screenshot(path="samplab_error_screenshot.png")
            print("Screenshot saved to samplab_error_screenshot.png")
            return None
        finally:
            browser.close()


if __name__ == "__main__":
    from dotenv import load_dotenv
    load_dotenv()
    
    # Test with a sample audio file
    import sys
    if len(sys.argv) > 1:
        audio_path = sys.argv[1]
    else:
        audio_path = "recordings/fw_1767830257.568494.wav"  # Default test file
    
    if not os.path.exists(audio_path):
        print(f"Error: Audio file not found: {audio_path}")
        sys.exit(1)
    
    midi_content = convert_audio_to_midi(audio_path)
    if midi_content:
        print(f"Success! MIDI file is {len(midi_content)} bytes")
        # Optionally save to file for testing
        output_path = "test_output.mid"
        with open(output_path, 'wb') as f:
            f.write(midi_content)
        print(f"Saved MIDI file to {output_path}")
    else:
        print("Conversion failed.")

