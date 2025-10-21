# 📋 Guía de Deploy a AWS Elastic Beanstalk

## 🚀 Setup Inicial

### 1️⃣ Configurar AWS CLI

Ejecuta en tu terminal:

```bash
aws configure
```

Te pedirá 4 valores (ingresa cada uno y presiona Enter):

```
AWS Access Key ID [None]: TU_ACCESS_KEY_ID
AWS Secret Access Key [None]: TU_SECRET_ACCESS_KEY
Default region name [None]: us-east-2
Default output format [None]: json
```

---

### 2️⃣ Verificar configuración

```bash
aws sts get-caller-identity
```

Deberías ver tu **Account ID** y **User ARN**.

---

### 3️⃣ Instalar EB CLI

```bash
pip install awsebcli --upgrade --user
```

O si tienes problemas:

```bash
python -m pip install awsebcli --upgrade --user
```

---

### 4️⃣ Verificar instalación

```bash
eb --version
```

---

## 📦 Configuración del Proyecto

### 5️⃣ Navegar a la carpeta del proyecto

```bash
cd F:\Documents\Motete-Transensorial\local-test\src
```

---

### 6️⃣ Inicializar Elastic Beanstalk

```bash
eb init
```

**Responde las preguntas así:**
- **Region:** `us-east-2`
- **Application name:** `motete-transensorial`
- **Platform:** `Node.js`
- **Platform version:** (la más reciente, generalmente opción 1)
- **CodeCommit:** `n` (no)
- **SSH:** `n` (no, por ahora)

---

### 7️⃣ Crear archivo de configuración `.ebignore`

```bash
echo "node_modules/" > .ebignore
echo "*.log" >> .ebignore
echo ".env" >> .ebignore
echo "certs/*.key" >> .ebignore
```

---

## 🌐 Deploy a AWS

### 8️⃣ Crear environment en Elastic Beanstalk

```bash
eb create motete-production --single
```

> El flag `--single` usa un solo servidor (más barato, perfecto para empezar).

---

### 9️⃣ Esperar el deploy (toma 5-10 minutos)

Verás logs en tiempo real. Al finalizar verás:

```
INFO: Successfully launched environment: motete-production
```

---

### 🔟 Abrir la aplicación en el navegador

```bash
eb open
```

---

## 🛠️ Comandos Útiles

### Deploy de cambios

```bash
eb deploy
```

### Ver logs

```bash
eb logs
```

### Ver estado

```bash
eb status
```

### Configurar variables de entorno

```bash
eb setenv AWS_IOT_ENDPOINT=a38a842oqvrvuj-ats.iot.us-east-2.amazonaws.com
eb setenv AWS_IOT_CLIENT_ID=director_aws
```

### Terminar environment (para no gastar dinero)

```bash
eb terminate motete-production
```

---

## ⚠️ Certificados AWS IoT Core

**Importante:** Necesitas configurar los certificados ANTES de que la app funcione.

### Opción 1: Usar AWS Secrets Manager (recomendado)

```bash
aws secretsmanager create-secret --name motete/certs/ca --secret-string file://certs/AmazonRootCA1.pem
aws secretsmanager create-secret --name motete/certs/client --secret-string file://certs/director-certificate.pem.crt
aws secretsmanager create-secret --name motete/certs/key --secret-string file://certs/director-private.pem.key
```

### Opción 2: Variables de entorno (más simple para testing)

Usa el script `setup-eb-env.ps1`:

```powershell
.\setup-eb-env.ps1
```

Esto configurará automáticamente las variables de entorno con los certificados.