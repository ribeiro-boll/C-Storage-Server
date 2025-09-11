let selectedFile = null;
let clientIp = 'Detectando...';
let MAX_FILE_SIZE = calculateMaxFileSize(); // Limite dinâmico baseado no dispositivo

// Variável global para rastrear buffers que precisam ser liberados
let activeBuffers = new Set();

function isMobileDevice() {
    // Múltiplas verificações para detectar dispositivos móveis
    const userAgent = navigator.userAgent.toLowerCase();
    const mobileKeywords = [
        'android', 'webos', 'iphone', 'ipad', 'ipod', 
        'blackberry', 'windows phone', 'mobile'
    ];
    
    // Verifica user agent
    const isMobileUserAgent = mobileKeywords.some(keyword => 
        userAgent.includes(keyword)
    );
    
    // Verifica se tem touch
    const hasTouch = 'ontouchstart' in window || navigator.maxTouchPoints > 0;
    
    // Verifica tamanho da tela
    const isSmallScreen = window.screen.width <= 768 || window.screen.height <= 768;
    
    // Verifica orientação (só existe em mobile)
    const hasOrientation = typeof window.orientation !== 'undefined';
    
    // Verifica se é um tablet/mobile baseado em múltiplos fatores
    return isMobileUserAgent || (hasTouch && isSmallScreen) || hasOrientation;
}

function calculateMaxFileSize() {
    const isMobile = isMobileDevice();
    
    if (isMobile) {
        // Mobile: limite menor (700MB aproximadamente)
        const mobileLimit = (1024 * 1024 * 1024 * 7) / 10; // ~0.7 GB
        console.log('📱 Dispositivo móvel detectado - Limite:', formatFileSize(mobileLimit));
        return mobileLimit;
    } else {
        // PC: limite maior (1.9GB)
        const pcLimit = 1.9 * 1024 * 1024 * 1024; // 1.9 GB
        console.log('💻 Dispositivo desktop detectado - Limite:', formatFileSize(pcLimit));
        return pcLimit;
    }
}

// FUNÇÕES DE LIMPEZA DE MEMÓRIA (equivalente ao free() do C)
function freeMemoryBuffers() {
    console.log('🗑️ Liberando', activeBuffers.size, 'buffer(s) da memória');
    
    // Limpa todas as referências dos buffers ativos
    activeBuffers.clear();
    
    // Força garbage collection se disponível
    forceGarbageCollection();
}

function forceGarbageCollection() {
    // Tenta forçar garbage collection (funciona só em dev/debug)
    if (window.gc) {
        console.log('🧹 Forçando garbage collection...');
        window.gc();
    }
    
    // Força limpeza com técnicas alternativas
    if (performance && performance.memory) {
        console.log('📊 Memória atual:', formatMemoryInfo(performance.memory));
    }
}

function getMemoryInfo() {
    if (performance && performance.memory) {
        const memory = performance.memory;
        return {
            used: formatFileSize(memory.usedJSHeapSize),
            total: formatFileSize(memory.totalJSHeapSize),
            limit: formatFileSize(memory.jsHeapSizeLimit)
        };
    }
    return { used: 'N/A', total: 'N/A', limit: 'N/A' };
}

function formatMemoryInfo(memory) {
    return `Usada: ${formatFileSize(memory.usedJSHeapSize)} / Total: ${formatFileSize(memory.totalJSHeapSize)} / Limite: ${formatFileSize(memory.jsHeapSizeLimit)}`;
}

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
    // Limpa arquivo anterior da memória
    if (selectedFile) {
        console.log('🔄 Substituindo arquivo - limpando memória...');
        freeMemoryBuffers();
    }
    
    // Recalcula o limite caso o usuário tenha mudado de orientação/dispositivo
    MAX_FILE_SIZE = calculateMaxFileSize();
    
    if (file.size > MAX_FILE_SIZE) {
        const deviceType = isMobileDevice() ? 'móvel' : 'desktop';
        showStatus(
            `Arquivo muito grande para dispositivo ${deviceType}! ` +
            `Limite máximo: ${formatFileSize(MAX_FILE_SIZE)}. ` +
            `Seu arquivo: ${formatFileSize(file.size)}`, 
            'error'
        );
        return;
    }
    
    selectedFile = file;
    const fileId = generateUniqueId();
    const uploadTime = new Date().toLocaleString('pt-BR');
    document.getElementById('fileName').textContent = file.name;
    document.getElementById('fileSize').textContent = formatFileSize(file.size);
    document.getElementById('fileType').textContent = file.type || 'Desconhecido';
    document.getElementById('fileId').textContent = fileId;
    document.getElementById('uploadTime').textContent = uploadTime;
    document.getElementById('fileInfo').classList.add('show');
    
    // Mostra informação sobre o limite do dispositivo
    const deviceType = isMobileDevice() ? 'móvel' : 'desktop';
    const memInfo = getMemoryInfo();
    showStatus(
        `Arquivo selecionado (${deviceType} - limite: ${formatFileSize(MAX_FILE_SIZE)}) | Mem: ${memInfo.used}`, 
        'success'
    );
}

// VERSÃO CUSTOMIZADA PARA BACKEND C COM CONTROLE DE MEMÓRIA
async function uploadFile() {
    if (!selectedFile) {
        showStatus('Selecione um arquivo primeiro!', 'error');
        return;
    }

    const serverUrl = document.getElementById('serverUrl').value.trim();
    console.log('=== DEBUG UPLOAD ===');
    console.log('URL configurada:', serverUrl);
    console.log('URL final para upload:', `${serverUrl}/upload`);
    console.log('Dispositivo móvel:', isMobileDevice());
    console.log('Limite atual:', formatFileSize(MAX_FILE_SIZE));
    console.log('🧠 Memória inicial:', getMemoryInfo());

    if (!serverUrl) {
        showStatus('URL do servidor não detectada! Recarregue a página.', 'error');
        return;
    }

    const progressBar = document.getElementById('progressBar');
    const progressFill = document.getElementById('progressFill');
    const uploadBtn = document.getElementById('uploadBtn');
    
    progressBar.style.display = 'block';
    uploadBtn.disabled = true;
    uploadBtn.textContent = 'Processando...';

    let customRequest = null;

    try {
        // Libera memória antes de começar
        freeMemoryBuffers();
        
        uploadBtn.textContent = 'Preparando arquivo...';
        customRequest = await createCustomHttpRequest(selectedFile);
        
        uploadBtn.textContent = 'Enviando...';
        console.log('🧠 Memória após criar request:', getMemoryInfo());
        
        const xhr = new XMLHttpRequest();
        
        xhr.upload.addEventListener('progress', (e) => {
            if (e.lengthComputable) {
                const percentComplete = (e.loaded / e.total) * 100;
                progressFill.style.width = percentComplete + '%';
            }
        });
        
        xhr.addEventListener('load', () => {
            console.log('🧠 Memória após upload:', getMemoryInfo());
            
            if (xhr.status === 200) {
                showStatus('Arquivo enviado com sucesso! 🎉', 'success');
                resetForm();
            } else {
                showStatus(`Erro no upload: ${xhr.status} - ${xhr.statusText}`, 'error');
            }
            
            // Limpa a memória SEMPRE após o upload (sucesso ou erro)
            customRequest = null;
            resetUploadState();
        });
        
        xhr.addEventListener('error', () => {
            showStatus('Erro de conexão com o servidor', 'error');
            customRequest = null;
            resetUploadState();
        });
        
        xhr.addEventListener('abort', () => {
            showStatus('Upload cancelado', 'error');
            customRequest = null;
            resetUploadState();
        });
        
        xhr.open('POST', `${serverUrl}/upload`);
        xhr.setRequestHeader('Content-Type', 'application/octet-stream');
        xhr.setRequestHeader('X-File-Name', selectedFile.name);
        xhr.setRequestHeader('X-File-Size', selectedFile.size.toString());
        xhr.setRequestHeader('X-File-Id', document.getElementById('fileId').textContent);
        xhr.setRequestHeader('X-Client-IP', clientIp);
        xhr.setRequestHeader('X-Upload-Time', document.getElementById('uploadTime').textContent);
        xhr.setRequestHeader('X-Device-Type', isMobileDevice() ? 'mobile' : 'desktop');
        
        xhr.send(customRequest);

    } catch (error) {
        console.error('❌ Erro no upload:', error);
        showStatus(`Erro: ${error.message}`, 'error');
        
        // Limpa memória em caso de erro
        customRequest = null;
        freeMemoryBuffers();
        resetUploadState();
    }
}

async function createCustomHttpRequest(file) {
    console.log('🧠 Criando request - Memória antes:', getMemoryInfo());
    
    const fileId = document.getElementById('fileId').textContent;
    const uploadTime = document.getElementById('uploadTime').textContent;
    const deviceType = isMobileDevice() ? 'mobile' : 'desktop';
    
    const header = 
        `<<<HEADER_START>>>\n` +
        `FILENAME: ${file.name}\n` +
        `FILESIZE: ${file.size}\n` +
        `FILETYPE: ${file.type || 'application/octet-stream'}\n` +
        `FILEID: ${fileId}\n` +
        `CLIENTIP: ${clientIp}\n` +
        `UPLOADTIME: ${uploadTime}\n` +
        `DEVICETYPE: ${deviceType}\n` +
        `MAXFILESIZE: ${MAX_FILE_SIZE}\n` +
        `<<<HEADER_END>>>\n` +
        `<<<FILE_DATA_START>>>\n`;
    
    const footer = ``;
    
    // Libera buffers antigos antes de criar novos
    freeMemoryBuffers();
    
    let fileBuffer = null;
    let headerBytes = null;
    let footerBytes = null;
    let combinedBuffer = null;
    let combinedView = null;
    
    try {
        // Cria os buffers
        fileBuffer = await file.arrayBuffer();
        headerBytes = new TextEncoder().encode(header);
        footerBytes = new TextEncoder().encode(footer);
        
        const totalSize = headerBytes.length + fileBuffer.byteLength + footerBytes.length;
        combinedBuffer = new ArrayBuffer(totalSize);
        combinedView = new Uint8Array(combinedBuffer);
        
        // Monta o buffer final
        combinedView.set(headerBytes, 0);
        combinedView.set(new Uint8Array(fileBuffer), headerBytes.length);
        combinedView.set(footerBytes, headerBytes.length + fileBuffer.byteLength);
        
        // Registra o buffer para limpeza posterior
        activeBuffers.add(combinedBuffer);
        
        console.log('🧠 Request criado - Memória depois:', getMemoryInfo());
        
        // Libera referências intermediárias imediatamente
        fileBuffer = null;
        headerBytes = null;
        footerBytes = null;
        combinedView = null;
        
        // Força garbage collection se disponível
        forceGarbageCollection();
        
        return combinedBuffer;
        
    } catch (error) {
        // Limpa tudo em caso de erro
        fileBuffer = null;
        headerBytes = null;
        footerBytes = null;
        combinedBuffer = null;
        combinedView = null;
        freeMemoryBuffers();
        throw error;
    }
}

function resetUploadState() {
    document.getElementById('progressBar').style.display = 'none';
    document.getElementById('progressFill').style.width = '0%';
    document.getElementById('uploadBtn').disabled = false;
    document.getElementById('uploadBtn').textContent = '🚀 Fazer Upload';
    
    // IMPORTANTE: Libera a memória após o upload
    console.log('🧠 Limpando memória após upload...');
    freeMemoryBuffers();
}

function resetForm() {
    selectedFile = null;
    document.getElementById('fileInput').value = '';
    document.getElementById('fileInfo').classList.remove('show');
    
    // Libera memória ao resetar o form
    freeMemoryBuffers();
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

// Mostra informações do dispositivo detectado ao carregar
function showDeviceInfo() {
    const isMobile = isMobileDevice();
    const deviceType = isMobile ? 'móvel' : 'desktop';
    const limit = formatFileSize(MAX_FILE_SIZE);
    
    console.log(`🔍 Dispositivo detectado: ${deviceType}`);
    console.log(`📏 Limite de arquivo: ${limit}`);
    
    // Opcional: mostrar uma notificação discreta
    setTimeout(() => {
        const keepAliveInfo = isMobile ? ' | Sistema anti-sleep disponível' : '';
        showStatus(`Dispositivo ${deviceType} detectado - Limite: ${limit}${keepAliveInfo}`, 'info');
    }, 1000);
}

// Limpa a memória quando o usuário sai da página
window.addEventListener('beforeunload', () => {
    console.log('🧠 Limpando memória antes de sair da página...');
    freeMemoryBuffers();
});

document.addEventListener('DOMContentLoaded', () => {
    detectClientIp();
    autoDetectServer();
    showDeviceInfo();
});

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => {
        detectClientIp();
        autoDetectServer();
        showDeviceInfo();
    });
} else {
    detectClientIp();
    autoDetectServer();
    showDeviceInfo();
}