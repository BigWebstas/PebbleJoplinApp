FROM node:20-bookworm-slim

# The Joplin CLI is what actually speaks Joplin Server's sync protocol and handles E2EE
# decryption - the bridge just shells out to it (see src/joplinCli.js).
RUN npm install -g joplin

WORKDIR /app
COPY package.json ./
RUN npm install --omit=dev
COPY src ./src

ENV JOPLIN_PROFILE_DIR=/data/joplin-profile
VOLUME ["/data/joplin-profile"]

EXPOSE 8077
CMD ["node", "src/index.js"]
