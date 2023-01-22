#ifndef DEFINE_H
#define DEFINE_H

#define COLUMNS 6  // �c�̃}�X��
#define ROWS 6     // ���̃}�X��

// ���萔 = COLUMNS * ROWS - 4(�����z�u)
#define TURNS (COLUMNS * ROWS - 4)

// �i6x6���Ō�܂ŉ�͂���͎̂��Ԃ������肷����̂Łj�����̉�͂ȂǓr���Ő؂�グ�����Ƃ���
#if true  // ������ true �ɂ��Ď��s�������K�w��������
#undef TURNS
#define TURNS 20
static_assert((TURNS < COLUMNS * ROWS - 4), "TURNS is too large.");
#endif

#define SINGLE_HIERARCHEY_TOP 0  // �i�擪����j�V���O���X���b�h�ŏ�������K�w��
#define SINGLE_HIERARCHEY_BTM \
    10  // �i���[����j�V���O���X���b�h�ŏ�������K�w��

// �L���b�V����L��������K�w�̐[��
// �}���藦���Ⴏ��Ό����̃R�X�g�̂ق��������t���i6x6�Ō������L���ɂ��Ȃ��ق��������j
#define CACHED_HIERARCHEY 15

// ���s���J��Ԃ���
// 4x4�ȂǑ������Đ��m�Ɍv���ł��Ȃ��Ƃ�������܂킵�ĕ��ς��Ƃ�
#define NUMBER_OF_TRIALS 1

// �g�p����X���b�h�̐��C�傫�Ȓl���w�肵�Ă��g�p���Ă���CPU�̍ő�l�ȏ�ɂ͂Ȃ�Ȃ�
#define WORKER_THREAD_MAX 20

#define OPT_CACHE false  // ���ʂ��L���b�V������
#define OPT_SYMMETRY \
    false  // �L���b�V�������ɑΏ̐��𗘗p����i8�p�^�[���𓯂��Ƃ݂Ȃ����ƂŃL���b�V��������������j
#define OPT_MOVE_ORDERING_6x6 true  // 6x6�����̃��[�u�I�[�_�����O�e�[�u�����g��
#define OPT_ALPHA_BETA true  // �A���t�@�x�[�^�@�Ŏ}���肷��

// �v���ɉe�����o��悤�ȃf�o�b�O�\�����������߂̒�`
// ���i��true�ɂ��Ă������ق����m�F���₷������ǂ��ŏI�I�Ȍv����false�ōs��
#define DEBUG_PRINT true
#define CACHE_ANALYZE true  // �L���b�V���̃q�b�g����\������

#endif  // DEFINE_H
