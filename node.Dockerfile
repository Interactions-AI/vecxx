ARG NODE_VERSION=lts
FROM node:${NODE_VERSION} as base

WORKDIR /build

COPY . .
RUN npm install
RUN npm run build
RUN npm test -- --coverage

ENTRYPOINT ["node"]
