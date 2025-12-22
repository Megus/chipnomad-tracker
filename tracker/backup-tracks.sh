#!/bin/bash

# Sample bash script for convenient backup of tracks via SFTP

# Configuration variables
BACKUP_DEST="../../tracks/backups"
REMOTE_SERVER="sftp://muos@10.0.0.219:2022"
REMOTE_PATH="/SD2 (sdcard)/chiptune"
PASSWORD="muos"

# Create date folder
DATE_FOLDER=$(date +%Y-%m-%d)
BACKUP_DIR="$BACKUP_DEST/$DATE_FOLDER"
mkdir -p "$BACKUP_DIR"

# Download files via SFTP
sshpass -p "$PASSWORD" sftp -o StrictHostKeyChecking=no "$REMOTE_SERVER" << EOF
lcd $BACKUP_DIR
cd "$REMOTE_PATH"
mget -r *
quit
EOF

echo "Backup completed to: $BACKUP_DIR"