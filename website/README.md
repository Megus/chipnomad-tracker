# ChipNomad Website

This directory contains the Hugo website and AWS CDK infrastructure for chipnomad.org

## Structure

- `hugo/` - Hugo static site generator files
- `cdk/` - AWS CDK infrastructure code
- `lambdas/` - Lambda@Edge functions

## Development

### Website

```bash
cd hugo
hugo server
```

### Deployment

```bash
cd hugo
hugo

cd ../cdk
npm install
npm run build
cdk deploy
```

## Requirements

- Hugo
- Node.js
- AWS CLI configured
- AWS CDK CLI