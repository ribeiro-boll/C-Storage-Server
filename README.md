![os](https://camo.githubusercontent.com/e6d28433c0c1041770537fc7f5af3110f9d9cb0b8e8aded756769aebdba81135/68747470733a2f2f696d672e736869656c64732e696f2f62616467652f2d4c696e75782d677265793f6c6f676f3d6c696e7578)
![license](https://img.shields.io/badge/License-Unlicense-green)  ![badge](https://img.shields.io/badge/Lang-C-blue)

# 📁 C-Storage-Server  

![demo](www/2025-09-04 13-17-00.gif)


---

- *English version: [English version](#english)*  
- *Versão pt-BR: [Versão pt-BR](#pt-br)*  

---

## English  
The **C-Storage-Server** is a minimalist HTTP server written in **C**, designed to handle multiple simultaneous connections with threads and efficiently manage file uploads.  

It was built with a strong focus on **performance, stability, and code clarity**, successfully tested with file uploads of up to **1GB** without data loss or corruption.  
The server also supports serving static files (HTML, CSS, JS, favicon), making it a lightweight and reliable solution for file storage.  

### Overview  
C-Storage-Server is a lightweight **HTTP file storage server** capable of receiving uploads and serving static content.  
It was tested with files up to **1GB** without corruption, ensuring stability even under high load.  

This project was created to explore **network programming in C**, focusing on low-level socket operations, multithreading with pthreads, and secure file handling.  

### Features  
- Multithreaded HTTP server using **POSIX threads**  
- Support for **binary file uploads** (no truncation/corruption)  
- Serving of **static frontend files** (HTML, CSS, JS, favicon)  
- Automatic **IP and port detection** with `ifaddrs`  
- Robustness tested with large file transfers (1GB)  

---

## pt-BR  

### Visão Geral  
O **C-Storage-Server** é um servidor HTTP minimalista escrito em **C**, projetado para lidar com múltiplas conexões simultâneas usando threads e gerenciar uploads de arquivos de forma eficiente.  

O projeto foi desenvolvido com foco em **performance, estabilidade e clareza de código**, suportando uploads de até **1GB** sem perda ou corrupção de dados.  
Além disso, também pode servir arquivos estáticos (HTML, CSS, JS, favicon), tornando-se uma solução completa e leve para armazenamento de arquivos.  


### Funcionalidades  
- Servidor HTTP multithreaded com **POSIX threads**  
- Suporte a upload de arquivos binários (sem truncar/corromper dados)  
- Servidor de arquivos estáticos (HTML, CSS, JS, favicon)  
- Detecção automática de **IP e porta** com `ifaddrs`  
- Testado com upload de arquivos grandes (até 1GB)  
 
