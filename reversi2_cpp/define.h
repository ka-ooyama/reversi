#ifndef DEFINE_H
#define DEFINE_H

#define COLUMNS					4   // �c�̃}�X��
#define ROWS					6   // ���̃}�X��

// ���萔 = COLUMNS * ROWS - 4(�����z�u)

#define STOP_HIERARCHEY		    64  // (default : 64) �r���Ő؂�グ��萔�i6x6�͎��Ԃ������肷����̂�16���x�Ő؂�グ�ď�����͂���j

#define HIERARCHEY_SINGLE		3   // ���[���琔���ă}���`�X���b�h�����Ȃ��K�w��

// �}���`�X���b�h�ŃL���b�V����L��������K�w�̐[��
// �}���藦���Ⴏ��Ό����̃R�X�g�̂ق��������t���i6x6�Ō������L���ɂ��Ȃ��ق��������j
#define CACHED_HIERARCHEY		15

// ���s���J��Ԃ���
// 4x4�ȂǑ������Đ��m�Ɍv���ł��Ȃ��Ƃ�������܂킵�ĕ��ς��Ƃ�
#define NUMBER_OF_TRIALS		1

// �g�p����X���b�h�̐��C�傫�Ȓl���w�肵�Ă��g�p���Ă���CPU�̍ő�l�ȏ�ɂ͂Ȃ�Ȃ�
#define WORKER_THREAD_MAX		20

#define OPT_CACHE               false   // ���ʂ��L���b�V������
#define OPT_SYMMETRY            false   // �L���b�V�������ɑΏ̐��𗘗p����i8�p�^�[���𓯂��Ƃ݂Ȃ����ƂŃL���b�V��������������j
#define OPT_MOVE_ORDERING_6x6   true    // 6x6�����̃��[�u�I�[�_�����O�e�[�u�����g��
#define OPT_ALPHA_BETA          true    // �A���t�@�x�[�^�@�Ŏ}���肷��

// �v���ɉe�����o��悤�ȃf�o�b�O�\�����������߂̒�`
// ���i��true�ɂ��Ă������ق����m�F���₷������ǂ��ŏI�I�Ȍv����false�ōs��
#define DEBUG_PRINT				true
#define CACHE_ANALYZE           true    // �L���b�V���̃q�b�g����\������

#endif  // DEFINE_H
