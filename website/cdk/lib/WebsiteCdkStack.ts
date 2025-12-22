import * as cdk from 'aws-cdk-lib';
import { Construct } from 'constructs';
import { WebChipNomadCom } from './WebChipNomadCom';

export class WebsiteCdkStack extends cdk.Stack {
  constructor(scope: Construct, id: string, props?: cdk.StackProps) {
    super(scope, id, props);
    const website = new WebChipNomadCom(this, "WebChipNomadCom");
  }
}