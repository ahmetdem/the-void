# The Void: Linux Kernel Subsystem Exploration

A collection of Linux Kernel Modules (LKM) built from scratch on Fedora 43. This project explores the transition from User Space to Kernel Space (Ring 0) across four distinct subsystems: Character Devices, Input Emulation, Process Scheduling, and Networking.

**Environment:** Fedora 43 (Linux Kernel 6.x)

## Project Modules

### 1. The Void (`the_void.c`)
**Subsystem:** Character Devices & IPC
The core of the system. It acts as a persistent memory store and an inter-module communication hub.
* **Capabilities:**
    * Creates a read/write interface at `/dev/the_void`.
    * Manages safe memory boundaries between User/Kernel space.
    * **API Provider:** Exports symbol `is_ip_banned()` for other kernel modules to consume.

### 2. The Phantom (`the_phantom.c`)
**Subsystem:** Input Subsystem & Timers
A virtual hardware device that mimics human input.
* **Capabilities:**
    * Registers as a legitimate keyboard device (`EV_KEY`).
    * Uses Kernel Timers (`struct timer_list`) to schedule future events.
    * Simulates keystrokes via interrupts to the input bus.

### 3. The Reaper (`the_reaper.c`)
**Subsystem:** Process Management & Scheduling
A process lifecycle controller that interacts directly with the task scheduler.
* **Capabilities:**
    * Accepts a PID input via `/dev/the_reaper`.
    * Iterates the kernel process tree (`task_struct`).
    * Uses RCU (Read-Copy-Update) locks for safe list traversal.
    * Sends lethal signals (`SIGKILL`) directly from kernel space.

### 4. The Guardian (`the_guardian.c`)
**Subsystem:** Netfilter & Networking
A dynamic firewall hooking into the IPv4 network stack.
* **Capabilities:**
    * Hooks into `NF_INET_LOCAL_IN`.
    * Inspects raw socket buffers (`sk_buff`) and IP headers.
    * **Dynamic Filtering:** Queries `the_void` module to check if an incoming packet source is banned.
    * Drops packets silently before they reach the OS firewall.
    
    
### 5\. The Thief (`mouse_thief.c`)

**Subsystem:** USB Core & Input Subsystem
A USB device driver that claims ownership of a specific hardware device, bypassing the default OS driver.

  * **Capabilities:**
      * **Device Hijacking:** Registers a `usb_driver` with specific Vendor/Product IDs to detach the default `usbhid` driver.
      * **Direct Hardware Access:** Allocates DMA-capable memory and submits URBs (USB Request Blocks) to read raw interrupt data.
      * **Protocol Reverse Engineering:** Decodes non-standard byte streams from wireless receivers.
      * **Input Injection:** Maps raw signals to the Kernel Input Subsystem to control the cursor programmatically.


---

## Build & Usage

### Prerequisites
```bash
sudo dnf install @development-tools kernel-devel kernel-headers
````

### Compilation

The project uses a unified Makefile.

```bash
make
```

### Loading Order (Dependency Chain)

Because `the_guardian` depends on `the_void`, they must be loaded in order.

1.  **Load the Core:**

    ```bash
    sudo insmod the_void.ko
    sudo chmod 666 /dev/the_void
    ```

2.  **Load the Subsystems:**

    ```bash
    sudo insmod the_phantom.ko
    sudo insmod the_reaper.ko
    sudo insmod the_guardian.ko
    ```

### Interaction Examples

**Banning an IP (The Void + Guardian):**

```bash
# Set the ban list to localhost
echo "127.0.0.1" > /dev/the_void
# Try to ping (It will hang/fail)
ping 127.0.0.1
```

**Summoning the Ghost (The Phantom):**

  * Load the module and wait 5 seconds. It will automatically type text into the active window.

**Terminating a Process (The Reaper):**

```bash
# Find a PID
ps aux | grep sleep
# Send it to the Reaper
echo "12345" > /dev/reaper
```

**Hijacking a Mouse (The Thief):**

```bash
# Load the driver (Your specific mouse ID must be compiled in)
sudo insmod mouse_thief.ko
# Watch the kernel logs to see raw data streaming from the hardware
sudo dmesg -w
```

-----

## Safety Warning

These modules run in **Ring 0**. They have full access to system memory and hardware. A bug here does not cause a segmentation fault; it causes a Kernel Panic. Use with caution on production machines.
