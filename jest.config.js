const { resolve, join } = require('path');

const APP_ROOT = resolve('.');
const REPORTS_DIR = join(APP_ROOT, 'reports');

module.exports = {
    coverageDirectory: join(REPORTS_DIR, 'coverage'),
    coveragePathIgnorePatterns: ['/node_modules/', '/tests/', '/lib/'],
    coverageReporters: ['json-summary', 'text', 'text-summary', 'lcov'],
    coverageThreshold: {
        global: {
            branches: 10,
            functions: 10,
            lines: 10,
            statements: 10
        }
    },
    globals: {
        'ts-jest': {
            tsconfig: `${APP_ROOT}/tsconfig.json`,
            babelConfig: false
        }
    },
    moduleFileExtensions: ['ts', 'tsx', 'js', 'json'],
    testEnvironment: 'node',
    testRegex: '(/__tests__/.*|\\.(test|spec))\\.(ts|tsx)$',
    testURL: 'http://localhost',
    transform: {
        '\\.(ts|tsx)$': 'ts-jest'
    },
    verbose: true,
    testMatch: null,
    preset: 'ts-jest'
};
