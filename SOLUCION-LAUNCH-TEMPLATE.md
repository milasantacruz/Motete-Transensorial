# 🔧 Solución: Error de Launch Configuration en Elastic Beanstalk (Diciembre 2024)

## 📋 Descripción del Error

```
Creating Auto Scaling launch configuration named: awseb-e-urhfmvi9ex-stack-AWSEBAutoScalingLaunchConfiguration-WHDFsBIWPlJs failed.
Reason: The Launch Configuration creation operation is not available in your account. 
Use launch templates to create configuration templates for your Auto Scaling groups.
(Service: AutoScaling, Status Code: 400)
```

## 🔍 Análisis del Problema

**¿Qué está pasando?**
- AWS **deprecó las Launch Configurations** a partir del **1 de octubre de 2024**
- Las cuentas nuevas o cuentas sin Launch Configurations previas **deben usar Launch Templates**
- Elastic Beanstalk intenta crear una Launch Configuration pero AWS la rechaza

**¿Por qué sucede?**
- AWS está migrando todos los servicios a Launch Templates (más moderno y flexible)
- Las Launch Configurations son una tecnología legacy que está siendo eliminada
- Tu cuenta no tiene permisos para crear Launch Configurations (porque ya no están disponibles)

## ✅ Solución Rápida

### Opción 1: Usar el archivo de configuración (Ya creado) ✅

Ya existe un archivo de configuración en tu proyecto:
- **Ubicación:** `local-test/src/.ebextensions/02_launch_template.config`

Este archivo fuerza el uso de Launch Templates configurando:
- `RootVolumeType: gp3` (fuerza Launch Templates)
- `DisableIMDSv1: true` (también fuerza Launch Templates)

**Solo necesitas hacer deploy:**

```bash
cd F:\Documents\Motete-Transensorial\local-test\src
eb deploy
```

### Opción 2: Verificar/Actualizar la configuración

Si el archivo no existe o quieres personalizarlo, créalo:

**Ubicación:** `local-test/src/.ebextensions/02_launch_template.config`

```yaml
# Configuración para usar Launch Templates en lugar de Launch Configurations
option_settings:
  aws:autoscaling:launchconfiguration:
    # Establecer el tipo de volumen raíz como gp3 fuerza el uso de Launch Templates
    RootVolumeType: gp3
    # Tamaño del volumen raíz (ajusta según tus necesidades)
    RootVolumeSize: 20
    # Deshabilitar IMDSv1 (también fuerza Launch Templates)
    DisableIMDSv1: true
```

Luego:
```bash
eb deploy
```

## 🖥️ Solución mediante AWS Console

### Paso 1: Ir a Elastic Beanstalk
1. Abre la consola de AWS
2. Ve a **Elastic Beanstalk**
3. Selecciona tu aplicación: `aromatio`
4. Selecciona tu environment: `Aromatorio-env`

### Paso 2: Editar Configuración de Capacidad
1. En el menú lateral, haz clic en **Configuración**
2. Busca la sección **Capacidad**
3. Haz clic en **Editar**

### Paso 3: Configurar para usar Launch Templates

Tienes varias opciones (elige al menos una):

#### Opción A: Cambiar tipo de volumen raíz
1. En **Volumen raíz**, cambia el tipo de **gp2** a **gp3**
2. Guarda los cambios

#### Opción B: Habilitar instancias Spot
1. Marca la casilla **Habilitar instancias Spot**
2. Selecciona al menos dos tipos de instancias compatibles
3. Guarda los cambios

#### Opción C: Deshabilitar IMDSv1
1. En **Seguridad**, busca **IMDSv1**
2. Desmarca la opción (deshabilita IMDSv1)
3. Guarda los cambios

### Paso 4: Aplicar cambios
1. Haz clic en **Aplicar**
2. Espera a que se complete la actualización (5-10 minutos)

## 🔧 Solución mediante EB CLI

### Verificar configuración actual

```bash
cd F:\Documents\Motete-Transensorial\local-test\src
eb config
```

Esto abrirá un editor con la configuración actual. Busca la sección de `aws:autoscaling:launchconfiguration` y agrega:

```yaml
option_settings:
  aws:autoscaling:launchconfiguration:
    RootVolumeType: gp3
    RootVolumeSize: 20
    DisableIMDSv1: true
```

Guarda y cierra. EB CLI preguntará si quieres aplicar los cambios.

### O crear/editar el archivo directamente

```bash
# Asegúrate de estar en el directorio correcto
cd F:\Documents\Motete-Transensorial\local-test\src

# Verificar que existe .ebextensions
ls .ebextensions

# Si no existe, crearlo
mkdir -p .ebextensions

# Crear/editar el archivo de configuración
# (En Windows PowerShell, usa notepad o tu editor preferido)
notepad .ebextensions\02_launch_template.config
```

Pega este contenido:

```yaml
option_settings:
  aws:autoscaling:launchconfiguration:
    RootVolumeType: gp3
    RootVolumeSize: 20
    DisableIMDSv1: true
```

Luego:
```bash
eb deploy
```

## 📝 Opciones que fuerzan Launch Templates

Cualquiera de estas configuraciones fuerza el uso de Launch Templates:

| Opción | Descripción | Recomendado |
|--------|-------------|-------------|
| `RootVolumeType: gp3` | Usa volumen gp3 (más moderno) | ✅ Sí |
| `DisableIMDSv1: true` | Deshabilita IMDSv1 (más seguro) | ✅ Sí |
| `EnableSpot: true` | Habilita instancias Spot | ⚠️ Solo si necesitas ahorrar costos |
| `BlockDeviceMappings` con `gp3` | Configuración avanzada de volúmenes | ⚠️ Solo si necesitas configuración específica |

**Recomendación:** Usa `RootVolumeType: gp3` + `DisableIMDSv1: true` (ya configurado en tu proyecto).

## 🚀 Después de aplicar la solución

1. **Espera 1-2 minutos** para que los cambios se propaguen
2. **Haz deploy nuevamente:**

```bash
cd F:\Documents\Motete-Transensorial\local-test\src
eb deploy
```

3. **Verifica el estado:**

```bash
eb status
```

## 🔍 Verificar que funcionó

Después del deploy, verifica en la consola de AWS:

1. Ve a **Elastic Beanstalk** → Tu environment
2. Ve a **Configuración** → **Capacidad**
3. Deberías ver que ahora usa **Launch Templates** en lugar de Launch Configurations

O verifica en CloudFormation:

1. Ve a **CloudFormation**
2. Busca el stack de tu environment (nombre como `awseb-e-xxx-stack`)
3. En los recursos, deberías ver un **Launch Template** en lugar de una Launch Configuration

## ⚠️ Notas Importantes

1. **Fecha de deprecación:** AWS deprecó las Launch Configurations el 1 de octubre de 2024
2. **Cuentas nuevas:** Todas las cuentas nuevas deben usar Launch Templates
3. **Cuentas existentes:** Las cuentas con Launch Configurations previas pueden seguir usándolas temporalmente, pero se recomienda migrar
4. **Sin cambios en funcionalidad:** Launch Templates ofrecen las mismas funcionalidades pero con más opciones y flexibilidad

## 📚 Referencias

- [AWS Announcement: Launch Configuration Deprecation](https://aws.amazon.com/about-aws/whats-new/2024/10/amazon-ec2-auto-scaling-launch-configurations/)
- [AWS Elastic Beanstalk - Launch Templates](https://docs.aws.amazon.com/elasticbeanstalk/latest/dg/environment-cfg-autoscaling.html)
- [AWS Knowledge Center - Launch Template Migration](https://repost.aws/knowledge-center/elastic-beanstalk-launch-template)

## 🆘 Si el error persiste

1. **Verifica que el archivo de configuración existe:**
   ```bash
   ls local-test/src/.ebextensions/02_launch_template.config
   ```

2. **Verifica el contenido del archivo:**
   ```bash
   cat local-test/src/.ebextensions/02_launch_template.config
   ```

3. **Intenta hacer un rebuild completo:**
   ```bash
   eb rebuild
   ```

4. **Si nada funciona, crea un nuevo environment:**
   ```bash
   eb create nuevo-environment-name --single
   ```

