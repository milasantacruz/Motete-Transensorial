# 🔧 Solución: Environment en Estado CREATE_FAILED

## 📋 Descripción del Error

```
The stack "arn:aws:cloudformation:us-east-2:816081435169:stack/awseb-e-urhfmvi9ex-stack/xxx" 
associated with environment "Aromatorio-env" is in "CREATE_FAILED" state. 
Rebuild or Terminate your environment.
```

## 🔍 Análisis del Problema

**¿Qué está pasando?**
- Tu environment de Elastic Beanstalk está en estado **CREATE_FAILED**
- Esto significa que el stack de CloudFormation falló durante la creación inicial
- Probablemente falló por el error de **Launch Configuration** que vimos antes
- En este estado, **NO puedes hacer deploy** hasta que lo arregles

**¿Por qué sucede?**
- El environment se creó antes de tener la configuración de Launch Templates
- AWS rechazó la creación porque intentó usar Launch Configurations (deprecadas)
- El stack de CloudFormation quedó en un estado inválido

## ✅ Solución: Dos Opciones

Tienes dos opciones para resolver esto:

### Opción 1: Rebuild (Recomendado) 🔄

**Rebuild** reconstruye el environment desde cero pero **mantiene**:
- ✅ El nombre del environment
- ✅ La URL del environment
- ✅ Las variables de entorno
- ✅ La configuración guardada

**Cuándo usar:** Si quieres mantener el mismo environment y solo reconstruirlo.

### Opción 2: Terminate y Crear Nuevo 🗑️

**Terminate** elimina completamente el environment y luego puedes crear uno nuevo.

**Cuándo usar:** Si no te importa perder el environment actual o quieres empezar completamente desde cero.

---

## 🔄 Opción 1: Rebuild del Environment

### Paso 1: Verificar que tienes la configuración correcta

Asegúrate de que el archivo de configuración existe:

**Ubicación:** `local-test/src/.ebextensions/02_launch_template.config`

**Contenido debe ser:**
```yaml
option_settings:
  aws:autoscaling:launchconfiguration:
    RootVolumeType: gp3
    DisableIMDSv1: true
```

### Paso 2: Rebuild desde la Consola de AWS

1. **Ve a Elastic Beanstalk** en la consola de AWS
2. **Selecciona tu aplicación:** `aromatio`
3. **Selecciona tu environment:** `Aromatorio-env`
4. En el menú **Acciones** (Actions), haz clic en **Rebuild environment** (Reconstruir environment)
5. **Confirma** el rebuild
6. **Espera** 10-15 minutos mientras se reconstruye

### Paso 3: Verificar el Rebuild

Después del rebuild:
1. El environment debería estar en estado **Ready** (Listo)
2. Ve a **Configuración** → **Capacidad**
3. Verifica que ahora usa **Launch Templates** (tipo de volumen: gp3)

### Paso 4: Subir tu ZIP

Una vez que el environment esté en estado **Ready**:
1. Haz clic en **Upload and deploy**
2. Selecciona tu ZIP (que incluye `.ebextensions/02_launch_template.config`)
3. Haz clic en **Deploy**

---

## 🗑️ Opción 2: Terminate y Crear Nuevo

### Paso 1: Terminar el Environment Actual

**⚠️ ADVERTENCIA:** Esto eliminará completamente el environment. Asegúrate de tener respaldo de cualquier configuración importante.

1. **Ve a Elastic Beanstalk** en la consola de AWS
2. **Selecciona tu aplicación:** `aromatio`
3. **Selecciona tu environment:** `Aromatorio-env`
4. En el menú **Acciones** (Actions), haz clic en **Terminate environment** (Terminar environment)
5. **Escribe el nombre del environment** para confirmar: `Aromatorio-env`
6. Haz clic en **Terminate**
7. **Espera** 5-10 minutos mientras se elimina

### Paso 2: Verificar que el Environment se Eliminó

1. El environment debería desaparecer de la lista
2. Ve a **CloudFormation** y verifica que el stack también se eliminó

### Paso 3: Crear Nuevo Environment

Tienes dos opciones:

#### Opción A: Crear desde la Consola (Manual)

1. **Ve a Elastic Beanstalk** → Tu aplicación `aromatio`
2. Haz clic en **Create environment** (Crear environment)
3. Selecciona **Web server environment**
4. **Configuración básica:**
   - Environment name: `Aromatorio-env` (o el nombre que prefieras)
   - Domain: (déjalo vacío o usa uno personalizado)
   - Platform: **Node.js**
   - Platform branch: (la más reciente)
   - Platform version: (la más reciente)
5. **Preset:** Selecciona **Single instance (free tier eligible)** para ahorrar costos
6. Haz clic en **Create environment**
7. **Espera** 10-15 minutos

#### Opción B: Crear con EB CLI

```bash
cd F:\Documents\Motete-Transensorial\local-test\src
eb create Aromatorio-env --single
```

### Paso 4: Configurar Launch Templates

**IMPORTANTE:** Antes de subir tu ZIP, configura el environment para usar Launch Templates:

1. Ve a tu nuevo environment
2. Ve a **Configuración** → **Capacidad**
3. Haz clic en **Editar**
4. En **Volumen raíz**, cambia el tipo a **gp3**
5. O marca **Habilitar instancias Spot**
6. Guarda los cambios

### Paso 5: Subir tu ZIP

1. Haz clic en **Upload and deploy**
2. Selecciona tu ZIP (que incluye `.ebextensions/02_launch_template.config`)
3. Haz clic en **Deploy**

---

## 🖥️ Rebuild mediante EB CLI

Si tienes EB CLI configurado, puedes hacer rebuild desde la terminal:

```bash
cd F:\Documents\Motete-Transensorial\local-test\src
eb rebuild Aromatorio-env
```

Esto:
- Reconstruye el environment
- Aplica automáticamente la configuración de `.ebextensions/02_launch_template.config` si está en el código fuente

---

## 🔍 Verificar el Estado del Environment

### Desde la Consola

1. Ve a **Elastic Beanstalk** → Tu environment
2. El estado debe ser **Ready** (Listo) en verde
3. Si está en **Warning** (Amarillo) o **Severe** (Rojo), revisa los eventos

### Desde EB CLI

```bash
eb status
```

### Ver Eventos/Logs

Para entender qué falló:

1. Ve a tu environment en Elastic Beanstalk
2. Haz clic en la pestaña **Events** (Eventos)
3. Revisa los eventos recientes para ver el error específico
4. También puedes ver los logs de CloudFormation:
   - Ve a **CloudFormation**
   - Busca el stack `awseb-e-urhfmvi9ex-stack`
   - Revisa la pestaña **Events** para ver qué falló

---

## ⚠️ Recomendación

**Recomiendo usar REBUILD** porque:
- ✅ Mantiene el nombre y URL del environment
- ✅ Mantiene las variables de entorno
- ✅ Es más rápido que terminar y crear nuevo
- ✅ Menos riesgo de perder configuraciones

**Solo usa TERMINATE si:**
- El rebuild falla repetidamente
- Quieres cambiar el nombre del environment
- Quieres empezar completamente desde cero

---

## 📝 Checklist Antes de Rebuild/Terminate

Antes de hacer rebuild o terminate, verifica:

- [ ] El archivo `02_launch_template.config` existe en `.ebextensions/`
- [ ] El contenido del archivo es correcto (RootVolumeType: gp3, DisableIMDSv1: true)
- [ ] Tienes un ZIP listo con `.ebextensions` incluido
- [ ] Has guardado cualquier configuración importante (variables de entorno, etc.)
- [ ] Tienes tiempo para esperar 10-15 minutos

---

## 🚀 Después de Rebuild/Terminate

Una vez que el environment esté en estado **Ready**:

1. **Sube tu ZIP** con `.ebextensions/02_launch_template.config` incluido
2. **Espera** a que el deploy termine
3. **Verifica** que no hay errores
4. **Prueba** tu aplicación

---

## 🆘 Si el Rebuild Falla

Si el rebuild también falla:

1. **Revisa los eventos** en Elastic Beanstalk para ver el error específico
2. **Verifica los permisos IAM** (ver `SOLUCION-ERROR-IAM.md`)
3. **Verifica que la configuración** de Launch Templates esté correcta
4. **Intenta Terminate y crear nuevo** si el rebuild falla repetidamente

---

## 📚 Referencias

- [AWS Knowledge Center - Invalid State](https://repost.aws/knowledge-center/elastic-beanstalk-invalid-state)
- [AWS Elastic Beanstalk - Rebuild Environment](https://docs.aws.amazon.com/elasticbeanstalk/latest/dg/using-features.healthstatus.html)
- [AWS Elastic Beanstalk - Terminate Environment](https://docs.aws.amazon.com/elasticbeanstalk/latest/dg/using-features.terminating.html)

