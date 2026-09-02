#!/usr/bin/env python3
"""
Example usage of ptywrap Python bindings

Prerequisites:
    podman run -d --name mytest alpine sleep 3600
"""

import ptywrap
import time

def main():
    print("PTYwrap Python Bindings Example")
    print("=" * 40)
    print()

    # Create session
    print("Attaching to container 'mytest'...")
    try:
        session = ptywrap.PTYSession("mytest", rows=40, cols=150)
    except RuntimeError as e:
        print(f"Error: {e}")
        print("Make sure container 'mytest' is running:")
        print("  podman run -d --name mytest alpine sleep 3600")
        return 1

    print(f"Attached! Terminal size: {session.rows}x{session.cols}")
    print(f"Exec PID: {session.get_pid()}")
    print()

    # Wait for shell to start
    time.sleep(1)

    # Send commands
    print("Sending commands...")
    session.send_line("echo 'Hello from Python!'")
    time.sleep(0.5)

    session.send_line("ls -la /")
    time.sleep(1)

    session.send_line("pwd")
    time.sleep(0.5)

    # Read output
    print("\nTerminal output (first 10 rows):")
    print("-" * 40)
    for i in range(10):
        text = session.get_row_text(i)
        print(f"{i:2}: {text}")
    print("-" * 40)
    print()

    # Get cursor position
    row, col = session.get_cursor()
    print(f"Cursor position: row {row}, col {col}")
    print()

    # Get a specific cell
    print("Cell at (2, 0):")
    cell = session.get_cell(2, 0)
    print(f"  Char: '{cell['char']}'")
    print(f"  FG Color: {cell['fg_color']}")
    print(f"  BG Color: {cell['bg_color']}")
    print(f"  Attributes: {cell['attrs']}")
    if cell['attrs'] & ptywrap.ATTR_BOLD:
        print("    - BOLD")
    print()

    # Take a screenshot
    print("Taking markdown screenshot (rows 0-15)...")
    markdown = session.screenshot(start_row=0, end_row=15)
    print("\n--- Markdown Screenshot ---")
    print(markdown)
    print("--- End Screenshot ---")
    print()

    # Save screenshot to file
    with open("python_screenshot.md", "w") as f:
        f.write("# PTY Screenshot from Python\n\n")
        f.write(markdown)
    print("Screenshot saved to: python_screenshot.md")
    print()

    # Check if still alive
    if session.is_alive():
        print("Exec process is still running")
    else:
        print("Exec process has exited")

    print("\nDone!")
    return 0

if __name__ == "__main__":
    exit(main())
