#ifndef DEFINE_H
#define DEFINE_H

#define COLUMNS					4   // �c�̃}�X��
#define ROWS					6	// ���̃}�X��

// ���萔 = COLUMNS * ROWS - 4(�����z�u)

// �\�ߑł��Ă����萔�i�ő�l�F���萔�j
#define PRESET_HIERARCHEY		0	// 6x6�ł����I���̂�15�O��

// ���[���琔���ă}���`�X���b�h�����Ȃ��K�w��
#define HIERARCHEY_SINGLE		3

// �}���`�X���b�h�ŃL���b�V������������K�w�̐[���i�ő�l�F���萔�j
// �傫����������ƃ����������肸���z�������Ɋ��蓖�Ă��Ēx���Ȃ�
#define HIERARCHEY_CACHED		0

// ���s���J��Ԃ���
// 4x4�ȂǑ������Đ��m�Ɍv���ł��Ȃ��Ƃ�������܂킵�ĕ��ς��Ƃ�
#define NUMBER_OF_TRIALS		1

// �L���b�V�������ɑΏ̐��𗘗p����i8�p�^�[���𓯂��Ƃ݂Ȃ����ƂŃL���b�V��������������j
#define USE_SYMMETRY_OPTIMIZE   false   // ������ false �ɂ��Ă�������

// �v���ɉe�����o��悤�ȃf�o�b�O�\�����������߂̒�`
// ���i��true�ɂ��Ă������ق����m�F���₷������ǂ��ŏI�I�Ȍv����false�ōs��
#define DEBUG_PRINT				true

// �g�p����X���b�h�̐��C�傫�Ȓl���w�肵�Ă��g�p���Ă���CPU�̍ő�l�ȏ�ɂ͂Ȃ�Ȃ�
#define WORKER_THREAD_MAX		20

#define ANALYZE_NODE_HIERARCHEY false

// �A���t�@�x�[�^�@�Ŏ}���肷��
#define ALPHA_BETA              true

// 6x6�����̃��[�u�I�[�_�����O�e�[�u�����g��
#define MOVE_ORDERING_6x6       true

#endif  // DEFINE_H