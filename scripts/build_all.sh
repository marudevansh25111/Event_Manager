#!/bin/bash
# scripts/build_all.sh

set -e

echo "🔨 Building Event Manager System..."

# Check if we're in the right directory
if [ ! -f "scripts/build_all.sh" ]; then
    echo "❌ Please run this script from the event-manager root directory"
    exit 1
fi

# Set environment variables for macOS
if [[ "$OSTYPE" == "darwin"* ]]; then
    export CMAKE_PREFIX_PATH="/opt/homebrew/lib/cmake:$CMAKE_PREFIX_PATH"
    export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:$PKG_CONFIG_PATH"
    echo "🍎 macOS detected - setting environment variables"
fi

# Create build directories
echo "📁 Creating build directories..."
mkdir -p server/build
mkdir -p client/build

# Check dependencies
echo "🔍 Checking dependencies..."

# Check if websocketpp exists - remove it since we're using Boost.Beast
if [ -d "third_party/websocketpp" ]; then
    echo "🗑️  Removing WebSocket++ (using Boost.Beast instead)..."
    rm -rf third_party/websocketpp
fi

# We don't need websocketpp anymore - using Boost.Beast which comes with Boost
echo "✅ Using Boost.Beast for WebSocket support"

# Build server
echo ""
echo "🖥️  Building Server..."
cd server/build

echo "   📋 Running CMake for server..."
cmake -DCMAKE_BUILD_TYPE=Release ..

echo "   🔨 Compiling server..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

if [ -f "./event_server" ]; then
    echo "   ✅ Server built successfully: server/build/event_server"
else
    echo "   ❌ Server build failed"
    exit 1
fi

cd ../..

# Build client
echo ""
echo "🖼️  Building Client..."
cd client/build

echo "   📋 Running CMake for client..."
cmake -DCMAKE_BUILD_TYPE=Release ..

echo "   🔨 Compiling client..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

if [ -f "./event_client" ] || [ -f "./event_client.app/Contents/MacOS/event_client" ]; then
    echo "   ✅ Client built successfully: client/build/event_client"
else
    echo "   ❌ Client build failed"
    exit 1
fi

cd ../..

echo ""
echo "🎉 Build completed successfully!"
echo ""
echo "📋 Built components:"
echo "   📁 Server executable: server/build/event_server"
echo "   📁 Client executable: client/build/event_client"
echo ""
echo "🚀 To run the system:"
echo ""
echo "   Terminal 1 (Server):"
echo "   cd server/build"
echo "   ./event_server"
echo ""
echo "   Terminal 2 (Client):"
echo "   cd client/build"
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "   ./event_client                    # Command line"
    echo "   # OR"
    echo "   open event_client.app             # macOS app bundle"
else
    echo "   ./event_client"
fi
echo ""
echo "🌐 Default server address: ws://localhost:8080"
echo "💡 The server will create an SQLite database (events.db) automatically"
echo ""

# Create quick start script
cat > start_server.sh << 'EOF'
#!/bin/bash
cd "$(dirname "$0")/server/build"
echo "🚀 Starting Event Manager Server..."
echo "📍 Server will run on ws://localhost:8080"
echo "🛑 Press Ctrl+C to stop"
./event_server
EOF

cat > start_client.sh << 'EOF'
#!/bin/bash
cd "$(dirname "$0")/client/build"
echo "🚀 Starting Event Manager Client..."
if [[ "$OSTYPE" == "darwin"* ]] && [ -d "event_client.app" ]; then
    open event_client.app
else
    ./event_client
fi
EOF

chmod +x start_server.sh start_client.sh

echo "📝 Quick start scripts created:"
echo "   ./start_server.sh - Start the server"
echo "   ./start_client.sh - Start the client"
echo ""
echo "✅ Ready to use!"