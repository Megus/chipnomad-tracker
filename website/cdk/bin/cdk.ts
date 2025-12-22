#!/usr/bin/env node
import 'source-map-support/register';
import * as cdk from 'aws-cdk-lib';
import { WebsiteCdkStack } from '../lib/WebsiteCdkStack';

const app = new cdk.App();
new WebsiteCdkStack(app, 'ChipNomadWebsiteStack', {
  env: {
    account: process.env.CDK_DEFAULT_ACCOUNT,
    region: process.env.CDK_DEFAULT_REGION,
  },
});