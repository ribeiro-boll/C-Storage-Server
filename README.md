![os](https://camo.githubusercontent.com/e6d28433c0c1041770537fc7f5af3110f9d9cb0b8e8aded756769aebdba81135/68747470733a2f2f696d672e736869656c64732e696f2f62616467652f2d4c696e75782d677265793f6c6f676f3d6c696e7578)
![license](https://img.shields.io/badge/License-MIT-green)  ![badge](https://img.shields.io/badge/Lang-C-blue)

# 📁 C-Storage-Server  

![demo](www/demo.gif)  

---

- *English version: [English version](#english)*  
- *Versão pt-BR: [Versão pt-BR](#pt-br)*  

---

## English  

### Overview  
The **C-Storage-Server** is a minimalist HTTP server written in **C**, designed to handle multiple simultaneous connections with a **thread pool** and efficiently manage file uploads.  

It was built with a strong focus on **performance, stability, and code clarity**, successfully tested with files up to **1GB** (the upload size limit is hard coded to be **1.9GB**, otherwise the frontend's ArrayBuffer could overflow) without data loss or corruption.  
The server also supports serving static files (HTML, CSS, JS, favicon), making it a lightweight and reliable solution for file storage.  

### Features  
- HTTP server implemented from scratch using **TCP/IP sockets**  
- **Binary file uploads** with support for large files (no truncation/corruption)  
- **Thread pool architecture** with task queue and workers  
- **SQLite database** integration for file metadata management  
- **REST API** for storage operations (upload, listing, etc.)  
- Serving of **static frontend files** (HTML, CSS, JS, favicon)  
- Automatic **IP and port detection** with `ifaddrs`  
- Stable file uploads up to **1.9GB** with guaranteed data transmission

### Technologies  
- **C (ANSI C)**  
- **POSIX Threads (pthread)**  
- **SQLite3**  
- **TCP/IP sockets**  

### Security  
- Use of **prepared statements** with SQLite to prevent SQL Injection  
- Manual parsing of **HTTP multipart/form-data** (no external frameworks)  
- Concurrency control with **mutex + condition variables**  

### Next Steps  
- Implement a **web interface** for downloads  

---

## pt-BR  

### Visão Geral  
O **C-Storage-Server** é um servidor HTTP minimalista escrito em **C**, projetado para lidar com múltiplas conexões simultâneas com um **thread pool** e gerenciar uploads de arquivos de forma eficiente.  

O projeto foi desenvolvido com foco em **performance, estabilidade e clareza de código**, suportando uploads de mais de **1GB** (o limite do tamanho dos uploads foi hard-coded para ser até **1.9GB**, caso o contrario, o ArrayBuffer do frontend corre risco de estourar) sem perda ou corrupção de dados.  
Além disso, também pode servir arquivos estáticos (HTML, CSS, JS, favicon), tornando-se uma solução completa e leve para armazenamento de arquivos.  

### Funcionalidades  
- Servidor HTTP implementado do zero usando **sockets TCP/IP**  
- **Uploads de arquivos binários grandes** (sem truncar/corromper dados)  
- **Arquitetura com thread pool** e fila de tarefas com workers  
- Integração com **SQLite** para gerenciamento de metadados dos arquivos  
- **API REST** para operações de armazenamento (upload, listagem, etc.)  
- Servidor de arquivos estáticos (**HTML, CSS, JS, favicon**)  
- Detecção automática de **IP e porta** com `ifaddrs`  
- Aguenta uploads de até **1.9GB** com estabilidade e garantia de dados 

### Tecnologias  
- **C (ANSI C)**  
- **POSIX Threads (pthread)**  
- **SQLite3**  
- **Sockets TCP/IP**  

### Segurança  
- Uso de **prepared statements** no SQLite (evita SQL Injection)  
- Parsing manual de **HTTP multipart/form-data** (sem frameworks externos)  
- Controle de concorrência via **mutex + condition variables**  

### Próximos Passos  
- Criar uma **interface web** para download  
  
