import * as route53 from 'aws-cdk-lib/aws-route53';
import * as s3 from 'aws-cdk-lib/aws-s3';
import * as lambda from 'aws-cdk-lib/aws-lambda';
import * as cloudfront from 'aws-cdk-lib/aws-cloudfront';
import * as s3deploy from 'aws-cdk-lib/aws-s3-deployment';
import * as targets from 'aws-cdk-lib/aws-route53-targets';
import * as cloudfront_origins from 'aws-cdk-lib/aws-cloudfront-origins';
import { CfnOutput, Duration, RemovalPolicy, Stack } from 'aws-cdk-lib';
import * as iam from 'aws-cdk-lib/aws-iam';
import { Construct } from 'constructs';
import { Certificate, CertificateValidation } from 'aws-cdk-lib/aws-certificatemanager';

const domainName = "chipnomad.org";
const prefix = "ChipNomadCom"

export class WebChipNomadCom extends Construct {
  constructor(parent: Stack, name: string) {
    super(parent, name);

    const zone = route53.HostedZone.fromLookup(this, `${prefix}Zone`, { domainName: domainName });
    const cloudfrontOAI = new cloudfront.OriginAccessIdentity(this, `${prefix}CloudfrontOAI`, {
      comment: `OAI for ${name}`
    });

    new CfnOutput(this, 'Site', { value: `https://${domainName}` });

    // Content bucket
    const siteBucket = new s3.Bucket(this, `${prefix}SiteBucket`, {
      bucketName: domainName,
      publicReadAccess: false,
      blockPublicAccess: s3.BlockPublicAccess.BLOCK_ALL,
      removalPolicy: RemovalPolicy.RETAIN,
      autoDeleteObjects: false,
    });

    // Grant access to CloudFront
    siteBucket.addToResourcePolicy(new iam.PolicyStatement({
      actions: ['s3:GetObject'],
      resources: [siteBucket.arnForObjects('*')],
      principals: [new iam.CanonicalUserPrincipal(cloudfrontOAI.cloudFrontOriginAccessIdentityS3CanonicalUserId)]
    }));
    new CfnOutput(this, `${prefix}Bucket`, { value: siteBucket.bucketName });

    // TLS certificate
    const certificate = new Certificate(this, `${prefix}SiteCertificate`, {
      domainName: domainName,
      validation: CertificateValidation.fromDns(zone),
    });
    new CfnOutput(this, `${prefix}Certificate`, { value: certificate.certificateArn });

    // Redirect Lambda@Edge
    const redirectFunction = new cloudfront.experimental.EdgeFunction(this, `${prefix}Redirect`, {
      runtime: lambda.Runtime.NODEJS_LATEST,
      handler: 'redirect.handler',
      code: lambda.Code.fromAsset('../lambdas/redirect')
    });

    // CloudFront distribution
    const distribution = new cloudfront.Distribution(this, `${prefix}SiteDistribution`, {
      certificate: certificate,
      defaultRootObject: "index.html",
      domainNames: [domainName],
      minimumProtocolVersion: cloudfront.SecurityPolicyProtocol.TLS_V1_2_2021,
      errorResponses:[
        {
          httpStatus: 403,
          responseHttpStatus: 403,
          responsePagePath: '/error.html',
          ttl: Duration.minutes(30),
        }
      ],
      defaultBehavior: {
        origin: new cloudfront_origins.S3Origin(siteBucket, {originAccessIdentity: cloudfrontOAI}),
        compress: true,
        allowedMethods: cloudfront.AllowedMethods.ALLOW_GET_HEAD_OPTIONS,
        viewerProtocolPolicy: cloudfront.ViewerProtocolPolicy.REDIRECT_TO_HTTPS,
        edgeLambdas: [
          {
            eventType: cloudfront.LambdaEdgeEventType.ORIGIN_REQUEST,
            functionVersion: redirectFunction.currentVersion
          }
        ]
      }
    })

    new CfnOutput(this, `${prefix}DistributionId`, { value: distribution.distributionId });

    // Route53 alias record for the CloudFront distribution
    new route53.ARecord(this, `${prefix}SiteAliasRecord`, {
      recordName: domainName,
      target: route53.RecordTarget.fromAlias(new targets.CloudFrontTarget(distribution)),
      zone
    });

    // Deploy site contents to S3 bucket
    new s3deploy.BucketDeployment(this, `${prefix}DeployWithInvalidation`, {
      sources: [s3deploy.Source.asset('../hugo/public')],
      destinationBucket: siteBucket,
      distribution,
      distributionPaths: ['/*'],
    });
  }
}