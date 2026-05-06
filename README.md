# Mini Ransomware Lab Project

## 📌 Overview

This project is part of our study on **Common Vulnerabilities and Exposures (CVE)** and **Malware Development (MalDev)**.
The goal is to understand how ransomware operates at a conceptual and technical level by building a **controlled, educational prototype** in a lab environment.

> ⚠️ This project is strictly for **educational purposes in a controlled environment only**.
> It must NOT be used on real systems or without explicit authorization.

---

## 🎯 Objectives

* Understand the fundamentals of **ransomware behavior**
* Explore how malware interacts with:

  * File systems
  * Cryptographic functions
  * Network communication
* Study how vulnerabilities (CVEs) can be leveraged in attack chains
* Gain hands-on experience in **reverse engineering and defensive analysis**

---

## 🧠 Theoretical Background

### 1. What is a CVE?

A **Common Vulnerabilities and Exposures (CVE)** is a publicly disclosed security flaw.
Attackers often use CVEs as an **initial access vector** to deliver malicious payloads.

Example attack chain:

```
CVE Exploit → Initial Access → Payload Execution → Persistence → Ransomware Activity
```

---

### 2. What is Ransomware?

Ransomware is a type of malware that:

* Encrypts victim files
* Denies access to data
* Demands payment (ransom) for decryption

Core components:

* File discovery
* Encryption mechanism
* Key management
* Communication with attacker (C2 / key server)

---

### 3. Cryptography in Ransomware

Most modern ransomware uses a combination of:

* **Symmetric encryption (fast)**
  → Used to encrypt files (e.g., AES)

* **Asymmetric encryption (secure key exchange)**
  → Used to protect the encryption key (e.g., RSA)

Typical workflow:

```
Generate symmetric key → Encrypt files → Encrypt key with RSA → Send to server
```

---

### 4. Command & Control (C2)

A **key server / C2 server** is used to:

* Store encryption keys
* Receive victim information
* Control infected machines

In this lab:

* A simple Flask-based server simulates key exchange

---

## ⚙️ Technologies Used

* **C** → low-level socket & system interaction
* **Python (Flask)** → key server (C2 simulation)
* **Cryptography library** → RSA key generation & serialization
* **Pandas** → simple key storage/logging
* **Sockets (WinSock)** → network communication

---

## 🔬 Lab Workflow (Simplified)

1. Exploit simulation (optional, via CVE)
2. Payload execution on victim machine
3. File scanning and filtering
4. File encryption process
5. Key generation and encryption
6. Key sent to attacker-controlled server
7. Server stores keys for later decryption

---

## 🛡️ Defensive Perspective

This project also helps understand how to defend against ransomware:

* Patch known CVEs
* Monitor abnormal file access patterns
* Detect suspicious encryption behavior
* Analyze network traffic to unknown servers
* Use EDR/AV solutions

---

## ⚠️ Ethical and Legal Notice

This project is intended strictly for:

* Cybersecurity education
* Malware analysis training
* Controlled lab environments

Any misuse of this knowledge:

* May violate laws
* May cause serious damage

**Always obtain proper authorization before testing.**

---

## 📚 References

* MITRE CVE Database
* NIST Vulnerability Database
* Malware analysis reports (e.g., WannaCry, LockBit)

---

## 👨‍💻 Author Notes

This project is part of a hands-on approach to learning:

* Reverse Engineering
* Exploit Development
* Malware Analysis

The focus is on understanding **how attacks work** in order to **build better defenses**.
