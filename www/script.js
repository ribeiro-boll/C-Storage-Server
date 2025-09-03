let selectedFile = null;
let clientIp = 'Detectando...';

async function detectClientIp() {
    try {
        const response = await fetch('https://api.ipify.org?format=json');
        const data = await response.json();
        clientIp = data.ip;
        document.getElementById('clientIp').textContent = clientIp;
    } catch (error) {
        clientIp = 'Não detectado';
        document.getElementById('clientIp').textContent = clientIp;
    }
}
function autoDetectServer() {
    const serverUrlField = document.getElementById('serverUrl');
    
    if (serverUrlField.value && 
        !serverUrlField.value.includes('localhost') && 
        !serverUrlField.value.includes('127.0.0.1') &&
        serverUrlField.value.trim() !== '') {
        console.log('🎯 URL do servidor já configurada:', serverUrlField.value);
        return;
    }
    
    const currentOrigin = window.location.origin;
    serverUrlField.value = currentOrigin;
    console.log('🎯 URL do servidor auto-detectada:', currentOrigin);
    showStatus(`Servidor detectado automaticamente: ${currentOrigin}`, 'success');
}

function generateUniqueId() {
    return 'file_' + Date.now() + '_' + Math.random().toString(36).substr(2, 9);
}

function formatFileSize(bytes) {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

const uploadArea = document.getElementById('uploadArea');
const fileInput = document.getElementById('fileInput');

uploadArea.addEventListener('click', () => {
    fileInput.click();
});

uploadArea.addEventListener('dragover', (e) => {
    e.preventDefault();
    uploadArea.classList.add('dragover');
});

uploadArea.addEventListener('dragleave', () => {
    uploadArea.classList.remove('dragover');
});

uploadArea.addEventListener('drop', (e) => {
    e.preventDefault();
    uploadArea.classList.remove('dragover');
    const files = e.dataTransfer.files;
    if (files.length > 0) {
        handleFileSelection(files[0]);
    }
});

fileInput.addEventListener('change', (e) => {
    if (e.target.files.length > 0) {
        handleFileSelection(e.target.files[0]);
    }
});

function handleFileSelection(file) {
    selectedFile = file;
    const fileId = generateUniqueId();
    const uploadTime = new Date().toLocaleString('pt-BR');
    document.getElementById('fileName').textContent = file.name;
    document.getElementById('fileSize').textContent = formatFileSize(file.size);
    document.getElementById('fileType').textContent = file.type || 'Desconhecido';
    document.getElementById('fileId').textContent = fileId;
    document.getElementById('uploadTime').textContent = uploadTime;
    document.getElementById('fileInfo').classList.add('show');
}

// VERSÃO CUSTOMIZADA PARA BACKEND C
async function uploadFile() {
    if (!selectedFile) {
        showStatus('Selecione um arquivo primeiro!', 'error');
        return;
    }

    const serverUrl = document.getElementById('serverUrl').value.trim();
    console.log('=== DEBUG UPLOAD ===');
    console.log('URL configurada:', serverUrl);
    console.log('URL final para upload:', `${serverUrl}/upload`);

    if (!serverUrl) {
        showStatus('URL do servidor não detectada! Recarregue a página.', 'error');
        return;
    }

    const progressBar = document.getElementById('progressBar');
    const progressFill = document.getElementById('progressFill');
    const uploadBtn = document.getElementById('uploadBtn');
    
    progressBar.style.display = 'block';
    uploadBtn.disabled = true;
    uploadBtn.textContent = 'Enviando...';

    try {
        const customRequest = await createCustomHttpRequest(selectedFile);
        const xhr = new XMLHttpRequest();
        xhr.upload.addEventListener('progress', (e) => {
            if (e.lengthComputable) {
                const percentComplete = (e.loaded / e.total) * 100;
                progressFill.style.width = percentComplete + '%';
            }
        });
        xhr.addEventListener('load', () => {
            if (xhr.status === 200) {
                showStatus('Arquivo enviado com sucesso! 🎉', 'success');
                resetForm();
            } else {
                showStatus(`Erro no upload: ${xhr.status} - ${xhr.statusText}`, 'error');
            }
            resetUploadState();
        });
        xhr.addEventListener('error', () => {
            showStatus('Erro de conexão com o servidor', 'error');
            resetUploadState();
        });
        xhr.open('POST', `${serverUrl}/upload`);
        xhr.setRequestHeader('Content-Type', 'application/octet-stream');
        xhr.setRequestHeader('X-File-Name', selectedFile.name);
        xhr.setRequestHeader('X-File-Size', selectedFile.size.toString());
        xhr.setRequestHeader('X-File-Id', document.getElementById('fileId').textContent);
        xhr.setRequestHeader('X-Client-IP', clientIp);
        xhr.setRequestHeader('X-Upload-Time', document.getElementById('uploadTime').textContent);
        
        xhr.send(customRequest);

    } catch (error) {
        showStatus(`Erro: ${error.message}`, 'error');
        resetUploadState();
    }
}
async function createCustomHttpRequest(file) {
    const fileId = document.getElementById('fileId').textContent;
    const uploadTime = document.getElementById('uploadTime').textContent;
    const header = 
        `<<<HEADER_START>>>\n` +
        `FILENAME: ${file.name}\n` +
        `FILESIZE: ${file.size}\n` +
        `FILETYPE: ${file.type || 'application/octet-stream'}\n` +
        `FILEID: ${fileId}\n` +
        `CLIENTIP: ${clientIp}\n` +
        `UPLOADTIME: ${uploadTime}\n` +
        `<<<HEADER_END>>>\n` +
        `<<<FILE_DATA_START>>>\n`;
    const footer = ``;
    const fileBuffer = await file.arrayBuffer();
    const headerBytes = new TextEncoder().encode(header);
    const footerBytes = new TextEncoder().encode(footer);  
    const totalSize = headerBytes.length + fileBuffer.byteLength + footerBytes.length;
    const combinedBuffer = new ArrayBuffer(totalSize);
    const combinedView = new Uint8Array(combinedBuffer);
    combinedView.set(headerBytes, 0);
    combinedView.set(new Uint8Array(fileBuffer), headerBytes.length);
    combinedView.set(footerBytes, headerBytes.length + fileBuffer.byteLength);
    
    return combinedBuffer;
}
function resetUploadState() {
    document.getElementById('progressBar').style.display = 'none';
    document.getElementById('progressFill').style.width = '0%';
    document.getElementById('uploadBtn').disabled = false;
    document.getElementById('uploadBtn').textContent = '🚀 Fazer Upload';
}
function resetForm() {
    selectedFile = null;
    document.getElementById('fileInput').value = '';
    document.getElementById('fileInfo').classList.remove('show');
}
function showStatus(message, type) {
    const status = document.getElementById('status');
    status.textContent = message;
    status.className = `status ${type}`;
    status.style.display = 'block';
    setTimeout(() => {
        status.style.display = 'none';
    }, 5000);
}
document.addEventListener('DOMContentLoaded', () => {
    detectClientIp();
    autoDetectServer();
});
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => {
        detectClientIp();
        autoDetectServer();
    });
} else {
    detectClientIp();
    autoDetectServer();
}