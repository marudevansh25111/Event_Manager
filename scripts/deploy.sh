#!/bin/bash
# scripts/deploy.sh

set -e

echo "🚀 Deploying Event Manager to Cloud..."

# Build for production
echo "🔨 Building for production..."
./scripts/build_all.sh

# Build Docker image
echo "🐳 Building Docker image..."
docker build -f docker/Dockerfile.server -t event-manager-server:latest .

# Tag for deployment (replace with your registry)
REGISTRY="your-registry.com"
IMAGE_NAME="event-manager-server"
VERSION="1.0.0"

echo "🏷️  Tagging images..."
docker tag event-manager-server:latest $REGISTRY/$IMAGE_NAME:$VERSION
docker tag event-manager-server:latest $REGISTRY/$IMAGE_NAME:latest

echo "✅ Docker images built successfully!"
echo ""
echo "🌐 Deployment options:"
echo ""
echo "1. 📤 Push to Docker Registry:"
echo "   docker push $REGISTRY/$IMAGE_NAME:$VERSION"
echo "   docker push $REGISTRY/$IMAGE_NAME:latest"
echo ""
echo "2. 🏗️  Deploy with Docker Compose (local):"
echo "   docker-compose -f docker/docker-compose.yml up -d"
echo ""
echo "3. ☁️  Deploy to Cloud Platforms:"
echo ""
echo "   🔸 AWS ECS:"
echo "   - Push image to ECR"
echo "   - Create ECS task definition"
echo "   - Deploy to ECS cluster"
echo ""
echo "   🔸 Google Cloud Run:"
echo "   gcloud run deploy event-manager \\"
echo "     --image=gcr.io/PROJECT_ID/event-manager-server:latest \\"
echo "     --platform=managed \\"
echo "     --port=8080 \\"
echo "     --allow-unauthenticated"
echo ""
echo "   🔸 DigitalOcean App Platform:"
echo "   - Connect your repository"
echo "   - Set build command: docker build -f docker/Dockerfile.server"
echo "   - Set run command: ./event_server 8080"
echo ""
echo "   🔸 Heroku:"
echo "   heroku container:push web --app your-app-name"
echo "   heroku container:release web --app your-app-name"
echo ""
echo "4. 🔧 Manual Deployment:"
echo "   - Copy server/build/event_server to your server"
echo "   - Install dependencies on target system"
echo "   - Run: ./event_server 8080"
echo "   - Open firewall port 8080"
echo ""

# Example deployment commands (uncomment as needed)
echo "💡 Example deployment workflow:"
echo ""
echo "# For local testing with Docker:"
echo "docker run -p 8080:8080 event-manager-server:latest"
echo ""
echo "# For production with Docker Compose:"
echo "docker-compose -f docker/docker-compose.yml up -d"
echo ""

# Create production configuration
echo "📝 Creating production configurations..."

cat > docker/.env.production << 'EOF'
# Production Environment Variables
DB_PATH=/app/data/events.db
SERVER_PORT=8080
LOG_LEVEL=info

# SSL Configuration (optional)
# SSL_CERT_PATH=/app/ssl/cert.pem
# SSL_KEY_PATH=/app/ssl/key.pem

# CORS Settings
# ALLOWED_ORIGINS=https://yourdomain.com,https://www.yourdomain.com
EOF

cat > nginx.conf << 'EOF'
events {
    worker_connections 1024;
}

http {
    upstream event_server {
        server event-server:8080;
    }

    map $http_upgrade $connection_upgrade {
        default upgrade;
        '' close;
    }

    server {
        listen 80;
        server_name your-domain.com;

        location / {
            proxy_pass http://event_server;
            proxy_http_version 1.1;
            proxy_set_header Upgrade $http_upgrade;
            proxy_set_header Connection $connection_upgrade;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            proxy_set_header X-Forwarded-Proto $scheme;
        }
    }
}
EOF

echo "✅ Production files created:"
echo "   📄 docker/.env.production - Environment variables"
echo "   📄 nginx.conf - Nginx configuration for reverse proxy"
echo ""
echo "🔒 Security checklist for production:"
echo "   ☐ Set up SSL/TLS certificates"
echo "   ☐ Configure firewall rules"
echo "   ☐ Set up monitoring and logging"
echo "   ☐ Configure backup for SQLite database"
echo "   ☐ Set up domain name and DNS"
echo "   ☐ Configure environment variables"
echo ""
echo "📊 Monitoring endpoints (implement in production):"
echo "   /health - Health check endpoint"
echo "   /metrics - Application metrics"
echo "   /version - Application version info"
echo ""
echo "🎉 Deployment preparation complete!"
echo "Choose your deployment method from the options above."