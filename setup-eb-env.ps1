# Script para configurar variables de entorno en AWS Elastic Beanstalk
# Uso: .\setup-eb-env.ps1

Write-Host "🔧 Configurando variables de entorno en Elastic Beanstalk..." -ForegroundColor Cyan

# Leer certificados
$certsPath = "F:\Documents\Motete-Transensorial\certs"

# Verificar que existen los certificados
if (-not (Test-Path "$certsPath\AmazonRootCA1.pem")) {
    Write-Host "❌ Error: No se encontró AmazonRootCA1.pem en $certsPath" -ForegroundColor Red
    exit 1
}
if (-not (Test-Path "$certsPath\director-certificate.pem.crt")) {
    Write-Host "❌ Error: No se encontró director-certificate.pem.crt en $certsPath" -ForegroundColor Red
    exit 1
}
if (-not (Test-Path "$certsPath\director-private.pem.key")) {
    Write-Host "❌ Error: No se encontró director-private.pem.key en $certsPath" -ForegroundColor Red
    exit 1
}

Write-Host "✅ Certificados encontrados" -ForegroundColor Green

# Leer contenido de certificados (escapar caracteres especiales)
$caCert = (Get-Content "$certsPath\AmazonRootCA1.pem" -Raw) -replace '"', '\"' -replace "`n", "\n" -replace "`r", ""
$clientCert = (Get-Content "$certsPath\director-certificate.pem.crt" -Raw) -replace '"', '\"' -replace "`n", "\n" -replace "`r", ""
$privateKey = (Get-Content "$certsPath\director-private.pem.key" -Raw) -replace '"', '\"' -replace "`n", "\n" -replace "`r", ""

# Endpoint de AWS IoT (reemplazar con el tuyo)
$endpoint = "a38a842oqvrvuj-ats.iot.us-east-2.amazonaws.com"
$clientId = "director_aws"

Write-Host "📤 Configurando variables de entorno en EB..." -ForegroundColor Cyan

# Cambiar al directorio del proyecto
cd F:\Documents\Motete-Transensorial\local-test\src

# Configurar variables de entorno
eb setenv `
  AWS_IOT_ENDPOINT="$endpoint" `
  AWS_IOT_CLIENT_ID="$clientId" `
  AWS_CA_CERT="$caCert" `
  AWS_CLIENT_CERT="$clientCert" `
  AWS_PRIVATE_KEY="$privateKey"

if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ Variables de entorno configuradas exitosamente" -ForegroundColor Green
    Write-Host ""
    Write-Host "🚀 Ahora ejecuta: eb deploy" -ForegroundColor Yellow
} else {
    Write-Host "❌ Error al configurar variables de entorno" -ForegroundColor Red
    exit 1
}

