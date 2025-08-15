#!/bin/bash
# scripts/install_deps_mac.sh

set -e

echo "🚀 Installing Event Manager Dependencies for macOS M3 Pro..."

# Install Homebrew (if not installed)
if ! command -v brew &> /dev/null; then
    echo "📦 Installing Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    
    # Add Homebrew to PATH for Apple Silicon
    echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
    eval "$(/opt/homebrew/bin/brew shellenv)"
else
    echo "✅ Homebrew already installed"
fi

# Update Homebrew
echo "🔄 Updating Homebrew..."
brew update

# Install required dependencies
echo "📚 Installing build dependencies..."
brew install cmake
brew install pkg-config
brew install sqlite3
brew install openssl
brew install nlohmann-json
brew install boost

# Install Qt6
echo "🎨 Installing Qt6..."
brew install qt6

# Install websocketpp (header-only library)
echo "🔌 Installing WebSocket++ library..."
if [ ! -d "./third_party/websocketpp" ]; then
    mkdir -p third_party
    cd third_party
    git clone https://github.com/zaphoyd/websocketpp.git
    cd ..
    echo "✅ WebSocket++ cloned to third_party/websocketpp"
else
    echo "✅ WebSocket++ already exists"
fi

# Verify installations
echo "🔍 Verifying installations..."

# Check CMAKE
if command -v cmake &> /dev/null; then
    echo "✅ CMake: $(cmake --version | head -n1)"
else
    echo "❌ CMake installation failed"
    exit 1
fi

# Check Qt6
if command -v qmake6 &> /dev/null; then
    echo "✅ Qt6: $(qmake6 -version | grep Qt)"
else
    echo "❌ Qt6 installation failed"
    exit 1
fi

# Check SQLite3
if command -v sqlite3 &> /dev/null; then
    echo "✅ SQLite3: $(sqlite3 --version)"
else
    echo "❌ SQLite3 installation failed"
    exit 1
fi

# Check if nlohmann-json is available
if brew list nlohmann-json &> /dev/null; then
    echo "✅ nlohmann-json: $(brew list --versions nlohmann-json)"
else
    echo "❌ nlohmann-json installation failed"
    exit 1
fi

# Check Boost
if brew list boost &> /dev/null; then
    echo "✅ Boost: $(brew list --versions boost)"
else
    echo "❌ Boost installation failed"
    exit 1
fi

# Set environment variables for the current session
export CMAKE_PREFIX_PATH="/opt/homebrew/lib/cmake:$CMAKE_PREFIX_PATH"
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:$PKG_CONFIG_PATH"

echo ""
echo "🎉 All dependencies installed successfully!"
echo ""
echo "📝 Environment setup:"
echo "   CMAKE_PREFIX_PATH: $CMAKE_PREFIX_PATH"
echo "   PKG_CONFIG_PATH: $PKG_CONFIG_PATH"
echo ""
echo "🚀 Next steps:"
echo "   1. Run: ./scripts/build_all.sh"
echo "   2. Start server: cd server/build && ./event_server"
echo "   3. Start client: cd client/build && ./event_client"
echo ""

# Add environment variables to shell profile
echo "💾 Adding environment variables to ~/.zprofile..."
if ! grep -q "CMAKE_PREFIX_PATH.*homebrew" ~/.zprofile 2>/dev/null; then
    echo 'export CMAKE_PREFIX_PATH="/opt/homebrew/lib/cmake:$CMAKE_PREFIX_PATH"' >> ~/.zprofile
fi

if ! grep -q "PKG_CONFIG_PATH.*homebrew" ~/.zprofile 2>/dev/null; then
    echo 'export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:$PKG_CONFIG_PATH"' >> ~/.zprofile
fi

echo "✅ Dependencies installation complete!"