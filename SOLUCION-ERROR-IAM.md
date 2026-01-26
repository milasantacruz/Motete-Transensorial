# 🔧 Solución: Error de Permisos IAM en Elastic Beanstalk

## 📋 Descripción del Error

```
Encountered a permissions error performing a tagging operation
User: arn:aws:sts::816081435169:assumed-role/aws-elasticbeanstalk-service-role/elasticbeanstalk 
is not authorized to perform: ec2:CreateTags on resource: arn:aws:ec2:us-east-2:816081435169:elastic-ip/*
```

## 🔍 Análisis del Problema

**¿Qué está pasando?**
- Elastic Beanstalk intenta crear una **Elastic IP** durante el deploy
- Al crear la Elastic IP, AWS intenta aplicar **tags** (etiquetas) al recurso
- El rol de servicio `aws-elasticbeanstalk-service-role` **no tiene permisos** para crear tags en recursos EC2

**¿Por qué sucede?**
- El rol de servicio de Elastic Beanstalk puede no tener todos los permisos necesarios
- Puede haber políticas IAM restrictivas en tu cuenta de AWS
- El rol puede no estar correctamente configurado

## ✅ Solución Rápida (AWS Console)

### Paso 1: Ir a IAM
1. Abre la consola de AWS
2. Ve a **IAM** (Identity and Access Management)
3. En el menú lateral, haz clic en **Roles**

### Paso 2: Buscar el Rol
1. Busca el rol: `aws-elasticbeanstalk-service-role`
2. Si no existe, ve al **Paso 4** para crearlo

### Paso 3: Agregar Permisos
1. Haz clic en el rol `aws-elasticbeanstalk-service-role`
2. Haz clic en la pestaña **Permisos**
3. Haz clic en **Agregar permisos** → **Adjuntar políticas**
4. Busca: `AmazonEC2FullAccess`
5. Selecciónala y haz clic en **Adjuntar políticas**

**⚠️ Nota:** `AmazonEC2FullAccess` da permisos completos a EC2. Si prefieres ser más restrictivo, ve al **Paso 5**.

### Paso 4: Crear el Rol (si no existe)
1. En IAM → Roles, haz clic en **Crear rol**
2. Selecciona **AWS service**
3. Selecciona **Elastic Beanstalk**
4. Selecciona **Elastic Beanstalk** como caso de uso
5. Haz clic en **Siguiente**
6. Adjunta estas políticas:
   - `AWSElasticBeanstalkService`
   - `AmazonEC2FullAccess` (o la política personalizada del Paso 5)
7. Nombre del rol: `aws-elasticbeanstalk-service-role`
8. Haz clic en **Crear rol**

### Paso 5: Política Personalizada (Más Segura)

Si prefieres no dar permisos completos a EC2, crea una política personalizada:

1. En IAM → **Políticas** → **Crear política**
2. Usa el editor JSON y pega esto:

```json
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": [
                "ec2:CreateTags",
                "ec2:DeleteTags",
                "ec2:DescribeTags"
            ],
            "Resource": "*"
        }
    ]
}
```

3. Nombra la política: `ElasticBeanstalkEC2Tagging`
4. Créala y luego adjúntala al rol `aws-elasticbeanstalk-service-role`

## 🖥️ Solución mediante AWS CLI

### Verificar el Rol Actual

```bash
aws iam get-role --role-name aws-elasticbeanstalk-service-role
```

### Opción A: Agregar Política Completa (Rápido)

```bash
aws iam attach-role-policy \
  --role-name aws-elasticbeanstalk-service-role \
  --policy-arn arn:aws:iam::aws:policy/AmazonEC2FullAccess
```

### Opción B: Crear Política Personalizada (Recomendado)

```bash
# Crear la política personalizada
aws iam create-policy \
  --policy-name ElasticBeanstalkEC2Tagging \
  --policy-document '{
    "Version": "2012-10-17",
    "Statement": [
      {
        "Effect": "Allow",
        "Action": [
          "ec2:CreateTags",
          "ec2:DeleteTags",
          "ec2:DescribeTags"
        ],
        "Resource": "*"
      }
    ]
  }'

# Obtener tu Account ID
ACCOUNT_ID=$(aws sts get-caller-identity --query Account --output text)

# Adjuntar la política al rol
aws iam attach-role-policy \
  --role-name aws-elasticbeanstalk-service-role \
  --policy-arn arn:aws:iam::${ACCOUNT_ID}:policy/ElasticBeanstalkEC2Tagging
```

### Opción C: Usar Política Inline (Más Simple)

```bash
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
          "ec2:DeleteTags",
          "ec2:DescribeTags"
        ],
        "Resource": "*"
      }
    ]
  }'
```

## 🚀 Después de Agregar Permisos

1. **Espera 1-2 minutos** para que los cambios de IAM se propaguen
2. **Vuelve a intentar el deploy:**

```bash
cd F:\Documents\Motete-Transensorial\local-test\src
eb deploy
```

## 🔍 Verificar que Funcionó

```bash
# Ver las políticas adjuntas al rol
aws iam list-attached-role-policies --role-name aws-elasticbeanstalk-service-role

# Ver políticas inline
aws iam list-role-policies --role-name aws-elasticbeanstalk-service-role
```

## 📚 Referencias

- [AWS Knowledge Center - CloudFormation Tagging Permission Error](https://repost.aws/knowledge-center/cloudformation-tagging-permission-error)
- [AWS Elastic Beanstalk Service Role](https://docs.aws.amazon.com/elasticbeanstalk/latest/dg/concepts-roles-service.html)
- [IAM Policy Reference - EC2 Actions](https://docs.aws.amazon.com/service-authorization/latest/reference/list_amazonec2.html)

## ⚠️ Notas Importantes

1. **Seguridad:** La política `AmazonEC2FullAccess` da permisos completos. Si tu organización requiere políticas más restrictivas, usa la Opción B o C.

2. **Propagación:** Los cambios de IAM pueden tardar 1-2 minutos en aplicarse. Si el error persiste, espera un poco más.

3. **Región:** Asegúrate de estar trabajando en la región correcta (`us-east-2` según tu configuración).

4. **Account ID:** Tu Account ID es `816081435169` (visible en el error).

