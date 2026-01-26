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

---

## 🔧 Solución de Errores Comunes

### ❌ Error: Permisos IAM - `ec2:CreateTags`

**Error:**
```
Encountered a permissions error performing a tagging operation
User: arn:aws:sts::ACCOUNT_ID:assumed-role/aws-elasticbeanstalk-service-role/elasticbeanstalk 
is not authorized to perform: ec2:CreateTags
```

**Causa:**
El rol de servicio de Elastic Beanstalk no tiene permisos para crear tags en recursos EC2 (especialmente Elastic IPs).

**Solución:**

#### Opción 1: Agregar permisos mediante AWS Console (Recomendado)

1. Ve a **IAM** en la consola de AWS
2. Busca el rol: `aws-elasticbeanstalk-service-role`
3. Haz clic en **Agregar permisos** → **Adjuntar políticas**
4. Busca y adjunta la política: `AmazonEC2FullAccess` (o crea una política personalizada más restrictiva)

#### Opción 2: Agregar permisos mediante AWS CLI

```bash
# Crear una política personalizada para tags
aws iam put-role-policy \
  --role-name aws-elasticbeanstalk-service-role \
  --policy-name ElasticBeanstalkEC2Tagging \
  --policy-document '{
    "Version": "2012-10-17",
    "Statement": [
      {
        "Effect": "Allow",
        "Action": [
          "ec2:CreateTags",
          "ec2:DeleteTags"
        ],
        "Resource": "*"
      }
    ]
  }'
```

#### Opción 3: Verificar y actualizar el rol de servicio

```bash
# Verificar el rol actual
aws iam get-role --role-name aws-elasticbeanstalk-service-role

# Si el rol no existe, crearlo con los permisos correctos
aws iam create-role \
  --role-name aws-elasticbeanstalk-service-role \
  --assume-role-policy-document '{
    "Version": "2012-10-17",
    "Statement": [
      {
        "Effect": "Allow",
        "Principal": {
          "Service": "elasticbeanstalk.amazonaws.com"
        },
        "Action": "sts:AssumeRole"
      }
    ]
  }'

# Adjuntar la política de servicio de Elastic Beanstalk
aws iam attach-role-policy \
  --role-name aws-elasticbeanstalk-service-role \
  --policy-arn arn:aws:iam::aws:policy/service-role/AWSElasticBeanstalkService

# Adjuntar permisos adicionales para EC2 tags
aws iam attach-role-policy \
  --role-name aws-elasticbeanstalk-service-role \
  --policy-arn arn:aws:iam::aws:policy/AmazonEC2FullAccess
```

**Nota:** Después de agregar los permisos, espera 1-2 minutos y vuelve a intentar el deploy:

```bash
eb deploy
```

---

### ❌ Error: Rol de servicio no encontrado

Si el rol `aws-elasticbeanstalk-service-role` no existe, créalo usando la consola de AWS:

1. Ve a **IAM** → **Roles** → **Crear rol**
2. Selecciona **AWS service** → **Elastic Beanstalk**
3. Selecciona **Elastic Beanstalk** como caso de uso
4. Adjunta las políticas necesarias:
   - `AWSElasticBeanstalkService`
   - `AmazonEC2FullAccess` (o una política personalizada con permisos de tags)
5. Crea el rol con el nombre: `aws-elasticbeanstalk-service-role`