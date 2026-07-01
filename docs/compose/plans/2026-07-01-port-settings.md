# Port Settings Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use compose:subagent (recommended) or compose:execute to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add port configuration to wizard and settings dialog, with auto-select (0) as default.

**Architecture:** Remove dead code, add port persistence, wire port through settings/wizard to SIP client.

**Tech Stack:** Qt6, PJSIP, C++

## Global Constraints

- Port 0 = auto-select (default)
- Port range: 0-65535
- Settings key: "port" in QSettings
- Transport port must be configurable (not hardcoded 5060)

---

### Task 1: Remove dead code in sipclient.cpp

**Covers:** Dead code cleanup

**Files:**
- Modify: `src/sipclient.cpp:193-248`

**Interfaces:**
- Produces: Clean sipclient.cpp without orphaned code

- [ ] **Step 1: Remove ensurePortBound() and orphaned code**

Remove lines 193-248 from sipclient.cpp. This includes:
- `ensurePortBound()` function (lines 193-210)
- Orphaned code block (lines 212-248) that was outside any function

After removal, the file should go directly from `registerAccount()` to the callback functions.

- [ ] **Step 2: Verify compilation**

Run: `cd /home/chirik/Projects/ChirikSIP && cmake --build build 2>&1 | head -30`

Expected: No errors related to missing functions or orphaned code.

- [ ] **Step 3: Commit**

```bash
git add src/sipclient.cpp
git commit -m "fix: remove dead code (ensurePortBound and orphaned makeCall block)"
```

---

### Task 2: Add port to loadSettings() and saveSettings()

**Covers:** Port persistence

**Files:**
- Modify: `src/mainwindow.cpp:158-178`
- Modify: `src/mainwindow.h:77` (already has `int m_port = 0;`)

**Interfaces:**
- Consumes: QSettings with key "port"
- Produces: m_port loaded from and saved to QSettings

- [ ] **Step 1: Update loadSettings()**

```cpp
void MainWindow::loadSettings()
{
    QSettings settings;
    m_server = settings.value("server").toString();
    m_username = settings.value("username").toString();
    m_password = settings.value("password").toString();
    m_port = settings.value("port", 0).toInt();
}
```

- [ ] **Step 2: Update saveSettings()**

```cpp
void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("server", m_server);
    settings.setValue("username", m_username);
    settings.setValue("password", m_password);
    settings.setValue("port", m_port);
    settings.sync();

    QFile configFile(settings.fileName());
    if (configFile.exists()) {
        configFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
    }
}
```

- [ ] **Step 3: Verify compilation**

Run: `cd /home/chirik/Projects/ChirikSIP && cmake --build build 2>&1 | head -30`

Expected: No errors.

- [ ] **Step 4: Commit**

```bash
git add src/mainwindow.cpp
git commit -m "feat: load and save port setting"
```

---

### Task 3: Wire port through onSettings()

**Covers:** Settings dialog port handling

**Files:**
- Modify: `src/mainwindow.cpp:560-585`

**Interfaces:**
- Consumes: SettingsDialog::setPort(), SettingsDialog::port()
- Produces: m_port updated from dialog, re-registration with port

- [ ] **Step 1: Update onSettings()**

```cpp
void MainWindow::onSettings()
{
    SettingsDialog dlg(this);
    dlg.setServer(m_server);
    dlg.setUsername(m_username);
    dlg.setPassword(m_password);
    dlg.setPort(m_port);

    if (dlg.exec() == QDialog::Accepted) {
        QString newServer = dlg.server();
        QString newUser = dlg.username();
        QString newPass = dlg.password();
        int newPort = dlg.port();

        bool changed = (newServer != m_server || newUser != m_username ||
                       newPass != m_password || newPort != m_port);

        m_server = newServer;
        m_username = newUser;
        m_password = newPass;
        m_port = newPort;
        saveSettings();

        if (changed && !m_server.isEmpty() && !m_username.isEmpty()) {
            m_statusLabel->setText("Re-registering...");
            m_statusLabel->setStyleSheet(STYLE_STATUS_REGISTERING);
            m_sipClient->registerAccount(m_server, m_username, m_password, m_port);
        }
    }
}
```

- [ ] **Step 2: Verify compilation**

Run: `cd /home/chirik/Projects/ChirikSIP && cmake --build build 2>&1 | head -30`

Expected: No errors.

- [ ] **Step 3: Commit**

```bash
git add src/mainwindow.cpp
git commit -m "feat: pass port from settings dialog to SIP client"
```

---

### Task 4: Wire port through onRegisterClicked()

**Covers:** Registration with port

**Files:**
- Modify: `src/mainwindow.cpp:381-391`

**Interfaces:**
- Consumes: m_port
- Produces: registerAccount() called with port

- [ ] **Step 1: Update onRegisterClicked()**

```cpp
void MainWindow::onRegisterClicked()
{
    if (m_server.isEmpty() || m_username.isEmpty()) {
        onSettings();
        return;
    }

    m_statusLabel->setText("Registering...");
    m_statusLabel->setStyleSheet(STYLE_STATUS_REGISTERING);
    m_sipClient->registerAccount(m_server, m_username, m_password, m_port);
}
```

- [ ] **Step 2: Verify compilation**

Run: `cd /home/chirik/Projects/ChirikSIP && cmake --build build 2>&1 | head -30`

Expected: No errors.

- [ ] **Step 3: Commit**

```bash
git add src/mainwindow.cpp
git commit -m "feat: pass port to registerAccount on initial registration"
```

---

### Task 5: Update SipClient to use configurable port

**Covers:** SIP transport port configuration

**Files:**
- Modify: `src/sipclient.h:21` (add port parameter to init)
- Modify: `src/sipclient.cpp:30-112` (use port in init)
- Modify: `src/mainwindow.cpp:142` (pass port to init)

**Interfaces:**
- Consumes: int port parameter
- Produces: SIP transport created on specified port

- [ ] **Step 1: Update sipclient.h**

Change `init()` signature:

```cpp
bool init(int port = 0);
```

Add member to store port:

```cpp
int m_transportPort = 0;
```

- [ ] **Step 2: Update sipclient.cpp init()**

```cpp
bool SipClient::init(int port)
{
    if (m_initialized)
        return true;

    m_transportPort = port;

    // ... existing pj_init, pjsua_create, pjsua_init code ...

    pjsua_transport_config transportCfg;
    pjsua_transport_config_default(&transportCfg);
    transportCfg.port = (port > 0) ? port : SIP_PORT;

    status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &transportCfg, nullptr);
    if (status != PJ_SUCCESS) {
        qCritical() << "Transport create failed:" << status;
        pjsua_destroy();
        return false;
    }

    // ... rest of init ...
}
```

- [ ] **Step 3: Update mainwindow.cpp to pass port**

In `MainWindow` constructor, change:

```cpp
m_sipClient->init(m_port);
```

- [ ] **Step 4: Verify compilation**

Run: `cd /home/chirik/Projects/ChirikSIP && cmake --build build 2>&1 | head -30`

Expected: No errors.

- [ ] **Step 5: Commit**

```bash
git add src/sipclient.h src/sipclient.cpp src/mainwindow.cpp
git commit -m "feat: use configurable port for SIP transport"
```

---

### Task 6: Full verification

**Covers:** End-to-end testing

**Files:**
- None (verification only)

**Interfaces:**
- Consumes: All previous changes
- Produces: Working build

- [ ] **Step 1: Full build**

Run: `cd /home/chirik/Projects/ChirikSIP && cmake --build build --clean-first 2>&1`

Expected: Build succeeds with no errors.

- [ ] **Step 2: Check for warnings**

Run: `cd /home/chirik/Projects/ChirikSIP && cmake --build build 2>&1 | grep -i warning`

Expected: No new warnings introduced.

- [ ] **Step 3: Final commit (if needed)**

```bash
git status
```

If there are uncommitted changes, commit them.
