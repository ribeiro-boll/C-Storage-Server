let selectedFile = null;
let clientIp = 'Detectando...';

// Detectar IP do cliente
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

// Gerar ID único
function generateUniqueId() {
    return 'file_' + Date.now() + '_' + Math.random().toString(36).substr(2, 9);
}

// Formatar tamanho do arquivo
function formatFileSize(bytes) {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

// Configurar eventos de drag and drop
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

// Manipular seleção de arquivo
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

// Função de upload
async function uploadFile() {
    if (!selectedFile) {
        showStatus('Selecione um arquivo primeiro!', 'error');
        return;
    }

    const serverUrl = document.getElementById('serverUrl').value;
    const progressBar = document.getElementById('progressBar');
    const progressFill = document.getElementById('progressFill');
    const uploadBtn = document.getElementById('uploadBtn');

    // Mostrar barra de progresso
    progressBar.style.display = 'block';
    uploadBtn.disabled = true;
    uploadBtn.textContent = 'Enviando...';

    try {
        // Criar FormData com as informações
        const formData = new FormData();
        formData.append('file', selectedFile);
        formData.append('fileId', document.getElementById('fileId').textContent);
        formData.append('clientIp', clientIp);
        formData.append('uploadTime', document.getElementById('uploadTime').textContent);
        formData.append('originalSize', selectedFile.size.toString());

        // Criar XMLHttpRequest para ter controle do progress
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
        xhr.send(formData);

    } catch (error) {
        showStatus(`Erro: ${error.message}`, 'error');
        resetUploadState();
    }
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

// Inicializar
detectClientIp();

// Exemplo de como fazer request raw HTTP/1.1 (para referência)
function createRawHttpRequest(file, metadata) {
    const boundary = '----WebKitFormBoundary' + Math.random().toString(36).substr(2);
    const CRLF = '\r\n';

    let body = '';

    // Adicionar metadados
    body += `--${boundary}${CRLF}`;
    body += `Content-Disposition: form-data; name="fileId"${CRLF}${CRLF}`;
    body += `${metadata.fileId}${CRLF}`;

    body += `--${boundary}${CRLF}`;
    body += `Content-Disposition: form-data; name="clientIp"${CRLF}${CRLF}`;
    body += `${metadata.clientIp}${CRLF}`;

    body += `--${boundary}${CRLF}`;
    body += `Content-Disposition: form-data; name="uploadTime"${CRLF}${CRLF}`;
    body += `${metadata.uploadTime}${CRLF}`;

    body += `--${boundary}${CRLF}`;
    body += `Content-Disposition: form-data; name="originalSize"${CRLF}${CRLF}`;
    body += `${metadata.originalSize}${CRLF}`;

    // Adicionar arquivo
    body += `--${boundary}${CRLF}`;
    body += `Content-Disposition: form-data; name="file"; filename="${file.name}"${CRLF}`;
    body += `Content-Type: ${file.type || 'application/octet-stream'}${CRLF}${CRLF}`;
    // Aqui viria o conteúdo binário do arquivo
    body += `${CRLF}--${boundary}--${CRLF}`;

    return {
        method: 'POST',
        headers: {
            'Content-Type': `multipart/form-data; boundary=${boundary}`,
            'Content-Length': body.length.toString(),
            'Host': '127.0.0.1:8080',
            'User-Agent': 'FileUploadClient/1.0'
        },
        body: body
    };
}
