from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import serialization
import struct
from flask import Flask, request
import pandas as pd
import zlib
import os
# source venv/bin/activate
app = Flask(__name__)

def rsa_to_windows_hex_blobs(key_size=2048):
    private_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=key_size
    )
    public_key = private_key.public_key()

    pub_nums = public_key.public_numbers()
    pub_header = struct.pack('<BBHI', 0x06, 0x02, 0, 0x0000a400)
    pub_struct = struct.pack('<I I I', 0x31415352, key_size, pub_nums.e)
    modulus = pub_nums.n.to_bytes(key_size // 8, byteorder='big')[::-1]
    
    public_blob_hex = (pub_header + pub_struct + modulus).hex()

    priv_nums = private_key.private_numbers()
    priv_header = struct.pack('<BBHI', 0x07, 0x02, 0, 0x0000a400)
    priv_struct = struct.pack('<I I I', 0x32415352, key_size, pub_nums.e)
    
    def to_le_bytes(val, size):
        return val.to_bytes(size, byteorder='big')[::-1]

    half_size = key_size // 16
    full_size = key_size // 8

    blob_parts = [
        priv_header,
        priv_struct,
        to_le_bytes(priv_nums.public_numbers.n, full_size), # Modulus
        to_le_bytes(priv_nums.p, half_size),               # P
        to_le_bytes(priv_nums.q, half_size),               # Q
        to_le_bytes(priv_nums.dmp1, half_size),            # DP
        to_le_bytes(priv_nums.dmq1, half_size),            # DQ
        to_le_bytes(priv_nums.iqmp, half_size),            # IQ
        to_le_bytes(priv_nums.d, full_size)                # Private Exponent
    ]
    
    private_blob_hex = b"".join(blob_parts).hex()

    return public_blob_hex, private_blob_hex

def save_keys_to_csv(id_hex, public_blob_hex, private_blob_hex, filename='keys.csv'):
    df = pd.DataFrame([{
        'id': id_hex,
        'pub': public_blob_hex,
        'priv': private_blob_hex
    }])
    
    # Kiểm tra xem file đã tồn tại chưa TRƯỚC KHI ghi
    file_exists = os.path.isfile(filename)
    
    # Luôn dùng mode='a' để ghi thêm vào cuối file
    # header=not file_exists: Nếu file chưa có thì ghi header, có rồi thì thôi
    df.to_csv(filename, index=False, mode='a', header=not file_exists)

@app.route('/', methods=['GET'])
def index():
    if request.args.get('id') is not None:
        id = request.args.get('id')
        df = pd.read_csv('keys.csv')
        row = df[df['id'] == id]
        if not row.empty:
            return row.iloc[0]['priv']
        else:
            return 'ID not found', 404
    else:
        id_hex = os.urandom(12).hex()
        public_blob_hex, private_blob_hex = rsa_to_windows_hex_blobs()
        save_keys_to_csv(id_hex, public_blob_hex, private_blob_hex)
        return id_hex + ':' + public_blob_hex

if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0', port=5001)