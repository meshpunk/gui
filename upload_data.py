import os

Import("env")

# Path to project's data directory
DATA_DIR = os.path.join(env.subst("$PROJECT_DIR"), "data")

def upload_filesystem(source, target, env):
    print("Uploading filesystem...")
    env.Execute("pio run --target uploadfs")

# Add a post-action to automatically upload the filesystem after firmware upload
env.AddPostAction("upload", upload_filesystem)