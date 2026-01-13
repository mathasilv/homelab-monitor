#!/bin/bash

# HomeLab Monitor Installation Script

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "Installing HomeLab Monitor..."
echo "Project directory: $PROJECT_DIR"

# Check Python version
PYTHON_VERSION=$(python3 --version 2>&1 | cut -d" " -f2 | cut -d"." -f1,2)
echo "Python version: $PYTHON_VERSION"

# Install Python dependencies
echo "Installing Python dependencies..."
cd "$PROJECT_DIR/server-client"
pip install -r requirements.txt

echo ""
echo "Installation complete!"
echo ""
echo "To run manually:"
echo "  cd $PROJECT_DIR/server-client"
echo "  python src/monitor.py /dev/ttyACM0"
echo ""
echo "To install as a systemd service:"
echo "  sudo cp $SCRIPT_DIR/homelab-monitor.service /etc/systemd/system/"
echo "  sudo systemctl daemon-reload"
echo "  sudo systemctl enable homelab-monitor"
echo "  sudo systemctl start homelab-monitor"
